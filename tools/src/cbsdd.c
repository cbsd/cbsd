/*
 * cbsdd.c — CBSD task daemon (C implementation)
 *
 * Заменяет shell-based cbsdd. Ключевое улучшение:
 * sqlite3_update_hook() вместо cbsd_fwatch + polling.
 *
 * При INSERT/UPDATE в таблицу taskd update_hook немедленно
 * просыпает dispatcher — нет задержки, нет polling, нет fork-per-query.
 *
 * Usage: cbsdd [options]
 *   --workdir <path>      Корневая директория CBSD (default: /usr/jails)
 *   --max-jobs N          Макс. параллельных задач (default: 60)
 *   --loop-timeout N      Таймаут проверки в секундах (default: 30)
 *   --foreground          Не daemonize
 *   --log-level N         Уровень логирования (0-3)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <pwd.h>
#include <grp.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/event.h>
#define USE_KQUEUE 1
#elif defined(__linux__)
#include <sys/inotify.h>
#define USE_INOTIFY 1
#endif

#define MAX_CMD_LEN 4096
#define MAX_PATH_LEN 512
#define MAX_RUNNING_JOBS 256
#define LOG_FILE_FMT    "%s/var/db/cbsdd.log"
#define PID_FILE_FMT    "%s/var/run/cbsdd.pid"
#define TASKD_DB_FMT    "%s/var/db/cbsdtaskd.sqlite"
#define NOTIFY_SOCK_FMT "%s/var/run/cbsdd.sock"

/* Task status codes (matching CBSD schema) */
#define STATUS_PENDING   0
#define STATUS_RUNNING   1
#define STATUS_COMPLETE  2

/* Log levels */
#define LOG_DEBUG   0
#define LOG_VERBOSE 1
#define LOG_NOTICE  2
#define LOG_WARNING 3

/* Configuration */
typedef struct {
    char workdir[MAX_PATH_LEN];
    char cbsd_path[MAX_PATH_LEN];
    char log_file[MAX_PATH_LEN];
    char socket_path[MAX_PATH_LEN];
    int max_simul_jobs;
    int loop_timeout;
    int foreground;
    int log_level;
} config_t;

/* Running job tracking */
typedef struct {
    pid_t pid;
    int task_id;
    time_t start_time;
} running_job_t;

/* Global state */
static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_wakeup = 0;
static sqlite3 *g_db = NULL;
static config_t g_config;
static running_job_t g_running_jobs[MAX_RUNNING_JOBS];
static int g_running_count = 0;
static int g_log_fd = -1;
static int g_listen_fd = -1;
static int g_watch_fd = -1;
static int g_watch_open_fd = -1;  /* kqueue: open fd of watched file */
static char g_sock_path[MAX_PATH_LEN] = {0};
static char g_watch_path[MAX_PATH_LEN] = {0};
#define MAX_CLIENT_FDS 64
static int g_client_fds[MAX_CLIENT_FDS];
static int g_client_count = 0;

static int write_full(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    while (len) {
        ssize_t n = write(fd, p, len);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return 0;
        p += (size_t)n;
        len -= (size_t)n;
    }
    return 1;
}

/* Persistent DB connection pool — linked list of open sqlite3 files.
 * Lazily checked for availability on db_pool_get(), not on every loop cycle. */
typedef struct db_pool_node {
    char path[MAX_PATH_LEN];
    sqlite3 *db;
    time_t opened_at;
    time_t last_used;
    uint64_t query_count;
    int is_taskd;
    struct db_pool_node *next;
} db_pool_node_t;

static db_pool_node_t *g_db_pool = NULL;
static int g_pool_size = 0;

static void cbsdlog(int level, const char *fmt, ...) {
    if (level < g_config.log_level) return;
    if (g_log_fd < 0) return;

    const char *level_str[] = {"DEBUG", "VERBOSE", "NOTICE", "WARNING"};
    char buf[2048];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    int off = strftime(buf, sizeof(buf), "%d %b %H:%M:%S", tm);

    va_list ap;
    va_start(ap, fmt);
    int n = snprintf(buf + off, sizeof(buf) - off, " [%s] ", level_str[level]);
    vsnprintf(buf + off + n, sizeof(buf) - off - n, fmt, ap);
    va_end(ap);

    size_t len = strlen(buf);
    if (len < sizeof(buf) - 1) {
        buf[len] = '\n';
        buf[len + 1] = '\0';
        len++;
    }

    write(g_log_fd, buf, len);
    if (level >= LOG_NOTICE) {
        write(STDERR_FILENO, buf, len);
    }
}

static void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            g_running = 0;
            g_wakeup = 1;
            break;
        case SIGCHLD:
            g_wakeup = 1;
            break;
    }
}

static void db_pool_log_state(void) {
    if (!g_db_pool) {
        cbsdlog(LOG_VERBOSE, "db_pool: empty (0 files)");
        return;
    }
    cbsdlog(LOG_VERBOSE, "db_pool: %d file(s) open:", g_pool_size);
    for (db_pool_node_t *n = g_db_pool; n; n = n->next) {
        time_t idle = time(NULL) - n->last_used;
        cbsdlog(LOG_VERBOSE, "  %s [%s] queries=%llu idle=%lus",
                n->path, n->is_taskd ? "taskd" : "cache",
                (unsigned long long)n->query_count, (unsigned long)idle);
    }
}

static int db_pool_file_ok(const char *path) {
    return (access(path, R_OK) == 0);
}

static db_pool_node_t *db_pool_find(const char *path) {
    for (db_pool_node_t *n = g_db_pool; n; n = n->next) {
        if (strcmp(n->path, path) == 0) return n;
    }
    return NULL;
}

static void db_pool_unlink_node(db_pool_node_t *node) {
    if (!node || !g_db_pool) return;

    if (g_db_pool == node) {
        g_db_pool = node->next;
    } else {
        db_pool_node_t *prev = g_db_pool;
        while (prev && prev->next != node) prev = prev->next;
        if (prev) prev->next = node->next;
    }
    g_pool_size--;
}

static int db_pool_open_node(db_pool_node_t *node, const char *path, int is_taskd) {
    int rc = sqlite3_open_v2(path, &node->db,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                              NULL);
    if (rc != SQLITE_OK) {
        cbsdlog(LOG_WARNING, "db_pool: cannot open %s: %s", path, sqlite3_errmsg(node->db));
        sqlite3_close_v2(node->db);
        node->db = NULL;
        return -1;
    }

    sqlite3_busy_timeout(node->db, 25000);
    sqlite3_exec(node->db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(node->db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(node->db, "PRAGMA mmap_size=209715200;", NULL, NULL, NULL);

    int persist = 1;
    sqlite3_file_control(node->db, NULL, SQLITE_FCNTL_PERSIST_WAL, &persist);

    snprintf(node->path, sizeof(node->path), "%s", path);
    node->opened_at   = time(NULL);
    node->last_used   = node->opened_at;
    node->query_count = 0;
    node->is_taskd    = is_taskd;
    node->next        = g_db_pool;
    g_db_pool         = node;
    g_pool_size++;

    cbsdlog(LOG_NOTICE, "db_pool: OPEN  %s [%s] pool_size=%d",
            path, is_taskd ? "taskd" : "cache", g_pool_size);
    return 0;
}

static void db_pool_close_node(db_pool_node_t *node) {
    if (!node) return;
    cbsdlog(LOG_NOTICE, "db_pool: CLOSE %s [%s] queries=%llu pool_size=%d",
            node->path, node->is_taskd ? "taskd" : "cache",
            (unsigned long long)node->query_count, g_pool_size - 1);
    sqlite3_close_v2(node->db);
    node->db = NULL;
}

/*
 * db_pool_get — получить persistent connection к sqlite3 файлу
 *
 * Ленивая проверка доступности: если файл уже в пуле —
 * проверяем access() только сейчас (не на каждый цикл event loop).
 * Если файл недоступен — удаляем из пула, логируем, пытаемся открыть заново.
 *
 * Возвращает: sqlite3* или NULL если файл недоступен.
 * out_is_new: 1 если соединение только что открыто, 0 если из кэша.
 */
static sqlite3 *db_pool_get(const char *path, int *out_is_new) {
    if (out_is_new) *out_is_new = 0;

    db_pool_node_t *node = db_pool_find(path);
    if (node) {
        /* File in pool — check availability now (lazy check) */
        if (!db_pool_file_ok(path)) {
            cbsdlog(LOG_NOTICE, "db_pool: UNAVAIL %s — file gone, removing", path);
            db_pool_unlink_node(node);
            db_pool_close_node(node);
            free(node);
            /* Fall through to try fresh open */
        } else {
            node->last_used = time(NULL);
            node->query_count++;
            cbsdlog(LOG_DEBUG, "db_pool: HIT %s queries=%llu", path, (unsigned long long)node->query_count);
            return node->db;
        }
    }

    /* Not in pool (or was removed) — check if file exists before opening */
    if (!db_pool_file_ok(path)) {
        cbsdlog(LOG_VERBOSE, "db_pool: MISS %s (not accessible)", path);
        return NULL;
    }

    node = calloc(1, sizeof(db_pool_node_t));
    if (!node) {
        cbsdlog(LOG_WARNING, "db_pool: OOM");
        return NULL;
    }

    if (db_pool_open_node(node, path, 0) != 0) {
        free(node);
        return NULL;
    }

    if (out_is_new) *out_is_new = 1;
    node->query_count = 1;
    return node->db;
}

static void db_pool_close_all(void) {
    db_pool_node_t *n = g_db_pool;
    while (n) {
        db_pool_node_t *next = n->next;
        db_pool_close_node(n);
        free(n);
        n = next;
    }
    g_db_pool = NULL;
    g_pool_size = 0;
}

/* Register an already-opened database into the pool (used for taskd) */
static void db_pool_register(const char *path, sqlite3 *db, int is_taskd) {
    db_pool_node_t *node = calloc(1, sizeof(db_pool_node_t));
    if (!node) return;

    snprintf(node->path, sizeof(node->path), "%s", path);
    node->db          = db;
    node->opened_at   = time(NULL);
    node->last_used   = node->opened_at;
    node->query_count = 0;
    node->is_taskd    = is_taskd;
    node->next        = g_db_pool;
    g_db_pool         = node;
    g_pool_size++;

    cbsdlog(LOG_NOTICE, "db_pool: REGISTER %s [%s] pool_size=%d",
            path, is_taskd ? "taskd" : "cache", g_pool_size);
}

static void cleanup(void) {
    if (g_db_pool) {
        db_pool_log_state();
        db_pool_close_all();
    }
    g_db = NULL;

    char pid_path[MAX_PATH_LEN];
    snprintf(pid_path, sizeof(pid_path), PID_FILE_FMT, g_config.workdir);
    unlink(pid_path);

    for (int i = 0; i < g_client_count; i++) {
        close(g_client_fds[i]);
    }
    g_client_count = 0;

    if (g_watch_fd >= 0) {
        close(g_watch_fd);
        g_watch_fd = -1;
    }
    if (g_watch_open_fd >= 0) {
        close(g_watch_open_fd);
        g_watch_open_fd = -1;
    }

    if (g_listen_fd >= 0) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }
    if (g_sock_path[0]) {
        unlink(g_sock_path);
    }

    if (g_log_fd >= 0) {
        close(g_log_fd);
        g_log_fd = -1;
    }
}

static int init_file_watch(const char *db_path) {
#if defined(USE_KQUEUE)
    g_watch_fd = kqueue();
    if (g_watch_fd < 0) {
        cbsdlog(LOG_WARNING, "kqueue() failed: %s", strerror(errno));
        return -1;
    }

    g_watch_open_fd = open(db_path, O_RDONLY);
    if (g_watch_open_fd < 0) {
        cbsdlog(LOG_WARNING, "Cannot open %s for watch: %s", db_path, strerror(errno));
        close(g_watch_fd);
        g_watch_fd = -1;
        return -1;
    }

    struct kevent change;
    EV_SET(&change, g_watch_open_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
        NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME,
        0, NULL);
    if (kevent(g_watch_fd, &change, 1, NULL, 0, NULL) < 0) {
        cbsdlog(LOG_WARNING, "kevent() failed: %s", strerror(errno));
        close(g_watch_open_fd);
        g_watch_open_fd = -1;
        close(g_watch_fd);
        g_watch_fd = -1;
        return -1;
    }

    snprintf(g_watch_path, sizeof(g_watch_path), "%s", db_path);
    cbsdlog(LOG_VERBOSE, "File watch (kqueue): %s", db_path);
    return 0;

#elif defined(USE_INOTIFY)
    g_watch_fd = inotify_init1(IN_NONBLOCK);
    if (g_watch_fd < 0) {
        cbsdlog(LOG_WARNING, "inotify_init() failed: %s", strerror(errno));
        return -1;
    }

    char wal_path[MAX_PATH_LEN];
    snprintf(wal_path, sizeof(wal_path), "%s-wal", db_path);

    int wd = inotify_add_watch(g_watch_fd, wal_path, IN_MODIFY | IN_CLOSE_WRITE);
    if (wd < 0) {
        cbsdlog(LOG_WARNING, "inotify_add_watch(%s) failed: %s, falling back to main db", wal_path, strerror(errno));
        wd = inotify_add_watch(g_watch_fd, db_path, IN_MODIFY | IN_CLOSE_WRITE);
        if (wd < 0) {
            cbsdlog(LOG_WARNING, "inotify_add_watch(%s) also failed: %s", db_path, strerror(errno));
            close(g_watch_fd);
            g_watch_fd = -1;
            return -1;
        }
        snprintf(g_watch_path, sizeof(g_watch_path), "%s", db_path);
    } else {
        snprintf(g_watch_path, sizeof(g_watch_path), "%s", wal_path);
    }

    cbsdlog(LOG_VERBOSE, "File watch (inotify): %s", g_watch_path);
    return 0;

#else
    cbsdlog(LOG_WARNING, "No file watch support on this platform");
    return -1;
#endif
}

static void drain_file_watch(void) {
#if defined(USE_INOTIFY)
    char evbuf[512];
    while (read(g_watch_fd, evbuf, sizeof(evbuf)) > 0) {}
#elif defined(USE_KQUEUE)
    struct kevent events[4];
    struct timespec zero = {0, 0};
    while (kevent(g_watch_fd, NULL, 0, events, 4, &zero) > 0) {}
#endif
}

/*
 * Protocol: client sends "<mode><db_path>\n<sql>\n"
 *   mode: R=read-only, W=read-write, J=JSON, V=vars
 *
 * Response: text lines + "END\n" on success, "ERR <msg>\n" on error
 *   R mode: "col1|col2|...|coln\n" per row
 *   W mode: "OK\n"
 *   J mode: "{\"col\":\"val\",...}\n" per row
 *   V mode: "C:col1,col2,...\n" then "val1|val2|...\n" per row
 */

static int send_line(int fd, const char *fmt, ...) {
    char buf[8192];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof(buf)) return -1;
    buf[n] = '\n';
    return write_full(fd, buf, (size_t)(n + 1));
}

static int read_line(int fd, char *buf, size_t maxlen) {
    size_t pos = 0;
    while (pos < maxlen - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) return -1;
        if (c == '\n') break;
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (int)pos;
}

static void handle_client_request(int client_fd) {
    char db_path[MAX_PATH_LEN] = {0};
    char sql[4096] = {0};

    /* Read: "<mode><db_path>\n<sql>\n" */
    char header[MAX_CMD_LEN] = {0};
    if (read_line(client_fd, header, sizeof(header)) < 0) {
        send_line(client_fd, "ERR protocol: no header");
        close(client_fd);
        return;
    }

    /* Parse mode — single character prefix */
    if (header[0] == 'N') {
        /* Notification only (from cbsd_task), drain and exit */
        close(client_fd);
        return;
    }

    char mode = header[0];
    if (mode != 'R' && mode != 'W' && mode != 'J' && mode != 'V') {
        send_line(client_fd, "ERR unknown mode '%c'", mode);
        close(client_fd);
        return;
    }

    /* db_path is the rest of the header after mode byte */
    snprintf(db_path, sizeof(db_path), "%s", header + 1);

    /* Read SQL query */
    if (read_line(client_fd, sql, sizeof(sql)) < 0) {
        send_line(client_fd, "ERR protocol: no sql");
        close(client_fd);
        return;
    }

    cbsdlog(LOG_DEBUG, "query: mode=%c db=%s sql=%.60s", mode, db_path, sql);

    /* Get connection from pool (lazy availability check) */
    int is_new = 0;
    sqlite3 *db = db_pool_get(db_path, &is_new);
    if (!db) {
        send_line(client_fd, "ERR cannot open %s", db_path);
        close(client_fd);
        return;
    }

    if (mode == 'W') {
        /* Read-write: execute, no result rows */
        char *errmsg = NULL;
        int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            send_line(client_fd, "ERR %s", errmsg ? errmsg : "exec failed");
            if (errmsg) sqlite3_free(errmsg);
        } else {
            send_line(client_fd, "OK");
        }
        send_line(client_fd, "END");
        close(client_fd);
        return;
    }

    /* Read-only modes: R (pipe-delimited), J (JSON), V (vars collect) */
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        send_line(client_fd, "ERR %s", sqlite3_errmsg(db));
        if (stmt) sqlite3_finalize(stmt);
        close(client_fd);
        return;
    }

    int ncols = sqlite3_column_count(stmt);
    int row_count = 0;

    if (mode == 'V') {
        /* V mode: first line is column names */
        char hdr[2048];
        int hoff = 0;
        for (int i = 0; i < ncols; i++) {
            const char *cn = sqlite3_column_name(stmt, i);
            if (i > 0) hdr[hoff++] = ',';
            int pl = snprintf(hdr + hoff, sizeof(hdr) - (size_t)hoff, "%s", cn);
            hoff += pl;
        }
        hdr[hoff] = '\0';
        send_line(client_fd, "C:%s", hdr);
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        row_count++;
        if (mode == 'J') {
            /* JSON row */
            char json[4096];
            int joff = 0;
            json[joff++] = '{';
            for (int i = 0; i < ncols; i++) {
                const char *cn = sqlite3_column_name(stmt, i);
                const char *cv = (const char *)sqlite3_column_text(stmt, i);
                if (i > 0) json[joff++] = ',';
                joff += snprintf(json + joff, sizeof(json) - (size_t)joff,
                                 "\"%s\":\"%s\"", cn, cv ? cv : "");
            }
            json[joff++] = '}';
            json[joff] = '\0';
            if (send_line(client_fd, "%s", json) < 0) break;
        } else {
            /* R and V modes: pipe-delimited */
            char row[4096];
            int roff = 0;
            for (int i = 0; i < ncols; i++) {
                const char *cv = (const char *)sqlite3_column_text(stmt, i);
                if (i > 0) row[roff++] = '|';
                roff += snprintf(row + roff, sizeof(row) - (size_t)roff, "%s", cv ? cv : "");
            }
            row[roff] = '\0';
            if (send_line(client_fd, "%s", row) < 0) break;
        }
    }
    sqlite3_finalize(stmt);

    send_line(client_fd, "END");
    cbsdlog(LOG_DEBUG, "query: mode=%c db=%s rows=%d", mode, db_path, row_count);
    close(client_fd);
}

static void reap_children(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        cbsdlog(LOG_DEBUG, "reap_children: pid=%d status=%d", pid, status);
        for (int i = 0; i < g_running_count; i++) {
            if (g_running_jobs[i].pid == pid) {
                int task_id = g_running_jobs[i].task_id;
                int errcode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
                cbsdlog(LOG_DEBUG, "reap_children: task=%d pid=%d errcode=%d", task_id, pid, errcode);

                /* Mark task as completed */
                char sql[512];
                char end_time[32];
                time_t now = time(NULL);
                struct tm *tm = localtime(&now);
                strftime(end_time, sizeof(end_time), "%Y%m%d%H%M%S", tm);

                snprintf(sql, sizeof(sql),
                    "UPDATE taskd SET status=%d, errcode=%d, end_time='%s' WHERE id=%d",
                    STATUS_COMPLETE, errcode, end_time, task_id);

                char *errmsg = NULL;
                if (sqlite3_exec(g_db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
                    cbsdlog(LOG_WARNING, "Failed to mark task %d complete: %s",
                            task_id, errmsg ? errmsg : "unknown");
                    sqlite3_free(errmsg);
                } else {
                    cbsdlog(LOG_NOTICE, "Task %d completed with errcode=%d", task_id, errcode);
                }

                /* Handle autoflush */
                sqlite3_stmt *stmt;
                snprintf(sql, sizeof(sql),
                    "SELECT autoflush,logfile FROM taskd WHERE id=%d", task_id);
                if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        int autoflush = sqlite3_column_int(stmt, 0);
                        const char *logfile = (const char *)sqlite3_column_text(stmt, 1);

                        int should_delete = 0;
                        if (autoflush == 1 && errcode == 0) should_delete = 1;
                        if (autoflush == 2) should_delete = 1;

                        if (should_delete) {
                            char delsql[256];
                            snprintf(delsql, sizeof(delsql),
                                "DELETE FROM taskd WHERE id=%d", task_id);
                            sqlite3_exec(g_db, delsql, NULL, NULL, NULL);

                            if (logfile && strcmp(logfile, "0") != 0) {
                                unlink(logfile);
                            }
                            cbsdlog(LOG_VERBOSE, "Auto-flushed task %d", task_id);
                        }
                    }
                    sqlite3_finalize(stmt);
                }

                /* Remove from running list */
                for (int j = i; j < g_running_count - 1; j++) {
                    g_running_jobs[j] = g_running_jobs[j + 1];
                }
                g_running_count--;
                break;
            }
        }
    }
}

/* SQLite update_hook callback — wakes up dispatcher on any taskd change */
static void update_hook_callback(void *arg, int op,
                                  const char *db_name,
                                  const char *table,
                                  sqlite_int64 rowid) {
    if (strcmp(table, "taskd") == 0) {
        cbsdlog(LOG_DEBUG, "update_hook: op=%d table=%s rowid=%lld", op, table, (long long)rowid);
        g_wakeup = 1;
    }
}

static int init_database(const char *db_path) {
    int rc = sqlite3_open_v2(db_path, &g_db,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                            NULL);
    if (rc != SQLITE_OK) {
        cbsdlog(LOG_WARNING, "Cannot open database: %s", sqlite3_errmsg(g_db));
        return -1;
    }

    sqlite3_busy_timeout(g_db, 25000);
    sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "PRAGMA mmap_size=209715200;", NULL, NULL, NULL);

    int persist = 1;
    sqlite3_file_control(g_db, NULL, SQLITE_FCNTL_PERSIST_WAL, &persist);

    cbsdlog(LOG_DEBUG, "init_database: busy_timeout=25000, WAL, synchronous=NORMAL, mmap=200MB, persist_wal=1");

    /* Register update_hook — this is the KEY improvement */
    sqlite3_update_hook(g_db, update_hook_callback, NULL);

    /* Crash recovery: reset orphaned running tasks */
    char *errmsg = NULL;
    rc = sqlite3_exec(g_db,
        "UPDATE taskd SET status=0 WHERE status=1",
        NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        cbsdlog(LOG_WARNING, "Crash recovery failed: %s", errmsg ? errmsg : "unknown");
        sqlite3_free(errmsg);
    } else {
        int changes = sqlite3_changes(g_db);
        if (changes > 0) {
            cbsdlog(LOG_NOTICE, "Crash recovery: reset %d orphaned tasks", changes);
        }
    }

    cbsdlog(LOG_NOTICE, "Database initialized: %s", db_path);

    /* Track taskd in the persistent connection pool */
    db_pool_register(db_path, g_db, 1);
    db_pool_log_state();

    return 0;
}

static int count_tasks(int status) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT COUNT(id) FROM taskd WHERE status=%d", status);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

/*
 * get_next_pending_task — получить следующую pending задачу с учётом зависимостей
 *
 * Аналог nexttask из shell-версии, но без fork:
 * 1. Найти задачу с status=0 (FIFO order)
 * 2. Проверить зависимость (after != 0)
 * 3. Если зависимость не выполнена — попробовать следующую
 * 4. Вернуть ID задачи для запуска, или -1 если нет готовых
 */
static int get_next_pending_task(char *cmd_out, size_t cmd_len,
                                  char *user_out, size_t user_len,
                                  char *logfile_out, size_t logfile_len,
                                  char *logtype_out, size_t logtype_len) {
    sqlite3_stmt *stmt;
    /* Get all pending tasks, ordered by ID */
    const char *sql =
        "SELECT id, cmd, user, logfile, logtype, after "
        "FROM taskd WHERE status=0 ORDER BY id LIMIT 100";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    int result_id = -1;
    int skipped = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *cmd = (const char *)sqlite3_column_text(stmt, 1);
        const char *user = (const char *)sqlite3_column_text(stmt, 2);
        const char *logfile = (const char *)sqlite3_column_text(stmt, 3);
        const char *logtype = (const char *)sqlite3_column_text(stmt, 4);
        int after = sqlite3_column_int(stmt, 5);

        if (!cmd || strcmp(cmd, "0") == 0) {
            cbsdlog(LOG_DEBUG, "next_task: id=%d skipped (no cmd)", id);
            continue;
        }

        /* Check dependency */
        if (after > 0) {
            cbsdlog(LOG_DEBUG, "next_task: id=%d depends on task %d", id, after);
            sqlite3_stmt *dep_stmt;
            char dep_sql[256];
            snprintf(dep_sql, sizeof(dep_sql),
                "SELECT status, errcode FROM taskd WHERE id=%d", after);

            if (sqlite3_prepare_v2(g_db, dep_sql, -1, &dep_stmt, NULL) == SQLITE_OK) {
                if (sqlite3_step(dep_stmt) == SQLITE_ROW) {
                    int dep_status = sqlite3_column_int(dep_stmt, 0);
                    int dep_errcode = sqlite3_column_int(dep_stmt, 1);

                    if (dep_status != STATUS_COMPLETE) {
                        /* Dependency not yet complete — skip */
                        cbsdlog(LOG_DEBUG, "next_task: id=%d blocked by dep %d (status=%d)", id, after, dep_status);
                        skipped++;
                        sqlite3_finalize(dep_stmt);
                        continue;
                    }
                    if (dep_errcode != 0) {
                        /* Dependency failed — mark this task as failed too */
                        char fail_sql[512];
                        char end_time[32];
                        time_t now = time(NULL);
                        struct tm *tm = localtime(&now);
                        strftime(end_time, sizeof(end_time), "%Y%m%d%H%M%S", tm);

                        snprintf(fail_sql, sizeof(fail_sql),
                            "UPDATE taskd SET status=%d, errcode=%d, end_time='%s' WHERE id=%d",
                            STATUS_COMPLETE, dep_errcode, end_time, id);
                        sqlite3_exec(g_db, fail_sql, NULL, NULL, NULL);
                        sqlite3_finalize(dep_stmt);
                        cbsdlog(LOG_NOTICE,
                            "Task %d skipped: dependency %d failed (errcode=%d)",
                            id, after, dep_errcode);
                        continue;
                    }
                }
                sqlite3_finalize(dep_stmt);
            }
        }

        /* Found a ready task */
        result_id = id;
        cbsdlog(LOG_DEBUG, "next_task: selected id=%d cmd='%s' user=%s after=%d (skipped=%d)",
                id, cmd, user, after, skipped);
        snprintf(cmd_out, cmd_len, "%s", cmd ? cmd : "");
        snprintf(user_out, user_len, "%s", user ? user : "root");
        snprintf(logfile_out, logfile_len, "%s", logfile ? logfile : "0");
        snprintf(logtype_out, logtype_len, "%s", logtype ? logtype : "auto");
        break;
    }

    sqlite3_finalize(stmt);
    return result_id;
}

/*
 * dispatch_task — запустить задачу как отдельный процесс
 *
 * Вместо spawn_task через shell, мы fork+exec напрямую.
 * Это эквивалент spawntask из CBSD, но встроенный.
 */
static void dispatch_task(int task_id, const char *cmd,
                          const char *user, const char *logfile,
                          const char *logtype) {
    if (g_running_count >= g_config.max_simul_jobs) {
        cbsdlog(LOG_VERBOSE, "Max jobs reached (%d), skipping dispatch", g_config.max_simul_jobs);
        return;
    }

    /* Determine actual logfile */
    char actual_logfile[MAX_PATH_LEN];
    if (strcmp(logtype, "auto") == 0) {
        snprintf(actual_logfile, sizeof(actual_logfile), "/tmp/taskd.%d.log", task_id);
    } else {
        snprintf(actual_logfile, sizeof(actual_logfile), "%s", logfile);
    }

    /* Mark task as running */
    char sql[512];
    char st_time[32];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(st_time, sizeof(st_time), "%Y%m%d%H%M%S", tm);

    snprintf(sql, sizeof(sql),
        "UPDATE taskd SET status=%d, st_time='%s', logfile='%s' WHERE id=%d",
        STATUS_RUNNING, st_time, actual_logfile, task_id);

    char *errmsg = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        cbsdlog(LOG_WARNING, "Failed to mark task %d as running: %s",
                task_id, errmsg ? errmsg : "unknown");
        sqlite3_free(errmsg);
        return;
    }

    cbsdlog(LOG_NOTICE, "Dispatching task %d: %s", task_id, cmd);

    pid_t pid = fork();
    if (pid < 0) {
        cbsdlog(LOG_WARNING, "fork() failed for task %d: %s", task_id, strerror(errno));
        return;
    }

    if (pid == 0) {
        /* Child process — execute the command */
        int log_fd;
        if (strcmp(actual_logfile, "0") != 0) {
            log_fd = open(actual_logfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        } else {
            log_fd = open("/dev/null", O_WRONLY);
        }

        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            if (log_fd > 2) close(log_fd);
        }

        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            if (devnull > 2) close(devnull);
        }

        setsid();

        /* Execute via cbsd -c (same as spawntask) */
        execl(g_config.cbsd_path, "cbsd", "-c", cmd, (char *)NULL);

        /* If execl failed */
        _exit(127);
    }

    /* Parent — track the running job */
    g_running_jobs[g_running_count].pid = pid;
    g_running_jobs[g_running_count].task_id = task_id;
    g_running_jobs[g_running_count].start_time = time(NULL);
    g_running_count++;

    cbsdlog(LOG_VERBOSE, "Task %d dispatched as PID %d", task_id, pid);
}

/*
 * dispatcher_loop — основной цикл dispatch
 *
 * Вместо polling через cbsd_fwatch + COUNT queries,
 * update_hook_callback устанавливает g_wakeup при каждом
 * INSERT/UPDATE в таблицу taskd.
 */
static void dispatcher_loop(void) {
    cbsdlog(LOG_NOTICE, "Dispatcher started (max_jobs=%d, timeout=%d)",
            g_config.max_simul_jobs, g_config.loop_timeout);
    cbsdlog(LOG_DEBUG, "dispatcher: entering main loop");

    while (g_running) {
        /* Reap completed children */
        reap_children();

        /* Check for pending tasks */
        int pending = count_tasks(STATUS_PENDING);
        int running = g_running_count;
        cbsdlog(LOG_DEBUG, "dispatcher: pending=%d running=%d wakeup=%d", pending, running, g_wakeup);

        if (pending <= 0 || running >= g_config.max_simul_jobs) {
            /* Nothing to do — wait for socket notification or timeout */
            g_wakeup = 0;
            cbsdlog(LOG_DEBUG, "dispatcher: sleeping (select timeout=%d)", g_config.loop_timeout);

            fd_set rfds;
            struct timeval tv;
            tv.tv_sec = g_config.loop_timeout;
            tv.tv_usec = 0;
            FD_ZERO(&rfds);
            int nfds = 0;

            if (g_listen_fd >= 0) {
                FD_SET(g_listen_fd, &rfds);
                nfds = g_listen_fd + 1;
            }
            if (g_watch_fd >= 0) {
                FD_SET(g_watch_fd, &rfds);
                if (g_watch_fd >= nfds) nfds = g_watch_fd + 1;
            }

            int ret = select(nfds, &rfds, NULL, NULL, &tv);
            if (ret > 0) {
                int notified = 0;
                if (g_listen_fd >= 0 && FD_ISSET(g_listen_fd, &rfds)) {
                    /* Accept and handle all pending client connections */
                    struct sockaddr_un remote;
                    socklen_t addr_len = sizeof(remote);
                    int client_fd;
                    while ((client_fd = accept(g_listen_fd, (struct sockaddr *)&remote, &addr_len)) >= 0) {
                        handle_client_request(client_fd);
                    }
                    notified = 1;
                    cbsdlog(LOG_DEBUG, "dispatcher: socket notifications drained");
                }
                if (g_watch_fd >= 0 && FD_ISSET(g_watch_fd, &rfds)) {
                    drain_file_watch();
                    notified = 1;
                    cbsdlog(LOG_DEBUG, "dispatcher: file watch triggered (db modified externally)");
                }
                if (notified) {
                    cbsdlog(LOG_DEBUG, "dispatcher: woke up (ret=%d)", ret);
                }
            } else {
                cbsdlog(LOG_DEBUG, "dispatcher: select returned %d (timeout or EINTR)", ret);
            }

            /* After wake-up, reap children again */
            reap_children();
            continue;
        }

        /* Dispatch tasks (up to max_simul_jobs) */
        int to_dispatch = pending;
        if (to_dispatch > g_config.max_simul_jobs - running) {
            to_dispatch = g_config.max_simul_jobs - running;
        }
        cbsdlog(LOG_DEBUG, "dispatcher: dispatching up to %d tasks", to_dispatch);

        for (int i = 0; i < to_dispatch && g_running; i++) {
            char cmd[MAX_CMD_LEN] = {0};
            char user[64] = {0};
            char logfile[MAX_PATH_LEN] = {0};
            char logtype[32] = {0};

            int task_id = get_next_pending_task(
                cmd, sizeof(cmd), user, sizeof(user),
                logfile, sizeof(logfile), logtype, sizeof(logtype));

            if (task_id <= 0) break;

            dispatch_task(task_id, cmd, user, logfile, logtype);

            /* Small delay between dispatches */
            usleep(100000); /* 100ms */
        }
    }

    /* Wait for all running jobs to finish */
    cbsdlog(LOG_NOTICE, "Shutting down, waiting for %d running jobs...", g_running_count);
    while (g_running_count > 0) {
        reap_children();
        if (g_running_count > 0) {
            usleep(500000); /* 500ms */
        }
    }
}

static void usage(const char *prog) {
    fprintf(stderr,
        "cbsdd ng: C-based reimplementatio of cbsdd\n\n"
        "Usage: %s [options]\n"
        "  --workdir <path>      CBSD root directory (default: /usr/jails)\n"
        "  --cbsd-path <path>    Path to cbsd binary (default: /usr/local/bin/cbsd)\n"
        "  --max-jobs N          Max parallel tasks (default: 60)\n"
        "  --loop-timeout N      Idle timeout in seconds (default: 30)\n"
        "  --foreground          Do not daemonize\n"
        "  --log-level N         Log level 0-3 (default: 2=NOTICE)\n"
        "  --debug               Shorthand for --log-level 0\n"
        "  --socket-path <path>  Unix socket path (default: <workdir>/var/run/cbsdd.sock)\n"
        "  --log-file <path>     Log file (default: <workdir>/var/db/cbsdd.log)\n"
        "  --help                This help\n"
        "\nLog levels:\n"
        "  0 = DEBUG    All events: socket wake, dispatch, reap, SQL hooks\n"
        "  1 = VERBOSE  Task lifecycle: dispatch, complete, flush\n"
        "  2 = NOTICE   Startup, shutdown, errors (default)\n"
        "  3 = WARNING  Errors only\n",
        prog);
}

int main(int argc, char *argv[]) {
    /* Defaults */
    strncpy(g_config.workdir, "/usr/jails", sizeof(g_config.workdir) - 1);
    strncpy(g_config.cbsd_path, "/usr/local/bin/cbsd", sizeof(g_config.cbsd_path) - 1);
    g_config.log_file[0] = '\0';
    g_config.socket_path[0] = '\0';
    g_config.max_simul_jobs = 60;
    g_config.loop_timeout = 30;
    g_config.foreground = 0;
    g_config.log_level = LOG_NOTICE;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--workdir") == 0 && i + 1 < argc) {
            strncpy(g_config.workdir, argv[++i], sizeof(g_config.workdir) - 1);
        } else if (strcmp(argv[i], "--cbsd-path") == 0 && i + 1 < argc) {
            strncpy(g_config.cbsd_path, argv[++i], sizeof(g_config.cbsd_path) - 1);
        } else if (strcmp(argv[i], "--max-jobs") == 0 && i + 1 < argc) {
            g_config.max_simul_jobs = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--loop-timeout") == 0 && i + 1 < argc) {
            g_config.loop_timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--foreground") == 0) {
            g_config.foreground = 1;
        } else if (strcmp(argv[i], "--debug") == 0) {
            g_config.log_level = LOG_DEBUG;
        } else if (strcmp(argv[i], "--socket-path") == 0 && i + 1 < argc) {
            strncpy(g_config.socket_path, argv[++i], sizeof(g_config.socket_path) - 1);
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            strncpy(g_config.log_file, argv[++i], sizeof(g_config.log_file) - 1);
        } else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            g_config.log_level = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--args") == 0) {
            fprintf(stderr, "cbsdd ng: C-based reimplementatio of cbsdd\n");
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* Open log */
    char log_path[MAX_PATH_LEN];
    if (g_config.log_file[0]) {
        snprintf(log_path, sizeof(log_path), "%s", g_config.log_file);
    } else {
        snprintf(log_path, sizeof(log_path), LOG_FILE_FMT, g_config.workdir);
    }
    g_log_fd = open(log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (g_log_fd < 0) {
        g_log_fd = open("/dev/null", O_WRONLY);
    }

    const char *level_names[] = {"DEBUG", "VERBOSE", "NOTICE", "WARNING"};
    cbsdlog(LOG_NOTICE, "cbsdd starting (C implementation), log_level=%s, log=%s",
            level_names[g_config.log_level], log_path);

    /* Signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    atexit(cleanup);

    /* Daemonize */
    if (!g_config.foreground) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid > 0) {
            /* Parent: write PID of child and exit */
            char pid_path[MAX_PATH_LEN];
            snprintf(pid_path, sizeof(pid_path), PID_FILE_FMT, g_config.workdir);
            FILE *f = fopen(pid_path, "w");
            if (f) {
                fprintf(f, "%d\n", pid);
                fclose(f);
            }
            return 0;
        }
        setsid();
    }

    /* Write PID */
    {
        char pid_path[MAX_PATH_LEN];
        snprintf(pid_path, sizeof(pid_path), PID_FILE_FMT, g_config.workdir);
        FILE *f = fopen(pid_path, "w");
        if (f) {
            fprintf(f, "%d\n", getpid());
            fclose(f);
        }
    }

    /* Initialize notification Unix socket */
    if (g_config.socket_path[0]) {
        snprintf(g_sock_path, sizeof(g_sock_path), "%s", g_config.socket_path);
    } else {
        snprintf(g_sock_path, sizeof(g_sock_path), NOTIFY_SOCK_FMT, g_config.workdir);
    }

    g_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_listen_fd < 0) {
        cbsdlog(LOG_WARNING, "Cannot create socket: %s", strerror(errno));
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", g_sock_path);

    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (errno != EADDRINUSE) {
            cbsdlog(LOG_WARNING, "Cannot bind socket %s: %s", g_sock_path, strerror(errno));
            close(g_listen_fd);
            g_listen_fd = -1;
            return 1;
        }

        /* EADDRINUSE — check if another daemon is actually listening */
        int probe_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (probe_fd < 0) {
            close(g_listen_fd);
            g_listen_fd = -1;
            return 1;
        }

        /* Set short timeout so we don't hang if socket is stale */
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        setsockopt(probe_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(probe_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        struct sockaddr_un probe_addr;
        memset(&probe_addr, 0, sizeof(probe_addr));
        probe_addr.sun_family = AF_UNIX;
        snprintf(probe_addr.sun_path, sizeof(probe_addr.sun_path), "%s", g_sock_path);

        int probe = connect(probe_fd, (struct sockaddr *)&probe_addr, sizeof(probe_addr));
        close(probe_fd);

        if (probe == 0) {
            /* Another daemon is actively listening — bail out */
            cbsdlog(LOG_WARNING, "Socket %s is already in use by another running cbsdd", g_sock_path);
            close(g_listen_fd);
            g_listen_fd = -1;
            return 1;
        }

        /* Nobody listening — stale socket, remove and retry */
        cbsdlog(LOG_NOTICE, "Stale socket %s detected, removing and retrying", g_sock_path);
        unlink(g_sock_path);

        if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            cbsdlog(LOG_WARNING, "Cannot bind socket %s after cleanup: %s", g_sock_path, strerror(errno));
            close(g_listen_fd);
            g_listen_fd = -1;
            return 1;
        }
    }

    /* Set socket permissions: owner=cbsd, group=cbsd, mode=0660 */
    struct passwd *pw = getpwnam("cbsd");
    struct group  *gr = getgrnam("cbsd");
    uid_t sock_uid = pw ? pw->pw_uid : getuid();
    gid_t sock_gid = gr ? gr->gr_gid : getgid();

    if (chown(g_sock_path, sock_uid, sock_gid) < 0) {
        cbsdlog(LOG_WARNING, "chown(%s, cbsd:cbsd) failed: %s", g_sock_path, strerror(errno));
    }
    if (chmod(g_sock_path, 0660) < 0) {
        cbsdlog(LOG_WARNING, "chmod(%s, 0660) failed: %s", g_sock_path, strerror(errno));
    }

    if (listen(g_listen_fd, 16) < 0) {
        cbsdlog(LOG_WARNING, "Cannot listen on socket: %s", strerror(errno));
        close(g_listen_fd);
        g_listen_fd = -1;
        unlink(g_sock_path);
        return 1;
    }

    /* Non-blocking accept */
    int flags = fcntl(g_listen_fd, F_GETFL, 0);
    fcntl(g_listen_fd, F_SETFL, flags | O_NONBLOCK);
    cbsdlog(LOG_NOTICE, "Notification socket: %s (uid=%d gid=%d 0660, fd=%d)",
            g_sock_path, (int)sock_uid, (int)sock_gid, g_listen_fd);

    /* Initialize database */
    char db_path[MAX_PATH_LEN];
    snprintf(db_path, sizeof(db_path), TASKD_DB_FMT, g_config.workdir);

    if (init_database(db_path) != 0) {
        cbsdlog(LOG_WARNING, "Failed to initialize database: %s", db_path);
        return 1;
    }

    /* Initialize file watch on database for external changes */
    init_file_watch(db_path);

    /* Preload main CBSD inventory database into pool */
    const char *preload_paths[] = {
        "%s/var/db/local.sqlite",        /* main inventory (symlink to inv.*.sqlite) */
        "%s/var/db/images.sqlite",       /* images */
        "%s/var/db/authkey.sqlite",      /* auth keys */
        "%s/var/db/nodes.sqlite",        /* nodes */
        "%s/var/db/storage_media.sqlite",/* storage */
        "%s/var/db/vpnet.sqlite",        /* vpnet */
        NULL
    };

    for (const char **p = preload_paths; *p; p++) {
        char pre_path[MAX_PATH_LEN];
        snprintf(pre_path, sizeof(pre_path), *p, g_config.workdir);

        int is_new = 0;
        sqlite3 *pre_db = db_pool_get(pre_path, &is_new);
        if (pre_db && is_new) {
            cbsdlog(LOG_VERBOSE, "Preloaded: %s", pre_path);
        }
    }

    db_pool_log_state();

    /* Main dispatcher loop */
    dispatcher_loop();

    cbsdlog(LOG_NOTICE, "cbsdd shutdown complete");
    return 0;
}
