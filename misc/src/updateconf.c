#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <time.h>

typedef struct {
    const char *arg;   // source argv[i]
    char *key;         // strings key
    const char *val;   // pointer to argv[i] after '='
    char *val_alloc;   // allocated value (if we had to join/unquote)
    int append;        // 1 when +=
    int applied;       // 1 when applied
} kv_arg_t;

static const char *skip_ws(const char *s)
{
	while (*s && isspace((unsigned char)*s))
		s++;
	return s;
}

static void rstrip_ws(char *s)
{
	size_t n = strlen(s);
	while (n > 0 && isspace((unsigned char)s[n - 1])) {
		s[n - 1] = '\0';
		n--;
	}
}

/*
 * Extract existing value from after the '=' sign in a config line.
 * Handles double-quoted, single-quoted and unquoted values.
 * Strips leading/trailing whitespace.
 */
static void extract_existing_value_after_eq(const char *after_eq, char *out, size_t maxlen)
{
	const char *s = skip_ws(after_eq);
	size_t len = 0;

	out[0] = '\0';
	if (maxlen == 0)
		return;

	if (*s == '"' || *s == '\'') {
		char q = *s++;
		const char *e = strchr(s, q);
		if (!e)
			e = s + strlen(s);
		len = (size_t)(e - s);
	} else {
		const char *e = s;
		while (*e && *e != '\n' && *e != '\r')
			e++;
		len = (size_t)(e - s);
	}

	if (len >= maxlen)
		len = maxlen - 1;
	memcpy(out, s, len);
	out[len] = '\0';
	rstrip_ws(out);
}

/*
 * Escape characters that are special inside double quotes:
 * backslash and double-quote itself.
 */
static void escape_for_double_quotes(const char *in, char *out, size_t outsz)
{
	size_t j = 0;
	for (size_t i = 0; in[i] && j + 1 < outsz; i++) {
		unsigned char c = (unsigned char)in[i];
		if ((c == '\\' || c == '"') && j + 2 < outsz) {
			out[j++] = '\\';
			out[j++] = (char)c;
		} else {
			out[j++] = (char)c;
		}
	}
	out[j] = '\0';
}

static int parse_kv_args(int argc, char *argv[], kv_arg_t **out_args, int *out_count) {
    kv_arg_t *args = calloc((size_t)argc, sizeof(*args));
    if (!args) return -1;

    int n = 0;
    for (int i = 0; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!eq) continue;

        int append = (eq > argv[i] && *(eq - 1) == '+');
        size_t key_len = append ? (size_t)(eq - argv[i] - 1) : (size_t)(eq - argv[i]);
        if (key_len == 0) continue;

        args[n].arg = argv[i];
        args[n].append = append;
        args[n].val = eq + 1;
        args[n].key = strndup(argv[i], key_len);
        if (!args[n].key) {
            for (int j = 0; j < n; j++) {
                free(args[j].key);
                free(args[j].val_alloc);
            }
            free(args);
            return -1;
        }

        /*
         * If caller passed an unquoted expansion like:
         *   pkglist="bash mc"
         * it can arrive split into argv as:
         *   pkglist="bash
         *   mc"
         * Here we detect a value starting with a quote (or escaped quote)
         * but missing its closing quote in the same argv element, and join
         * subsequent argv elements until the closing quote is found.
         */
        const char *v = args[n].val;
        char q = 0;
        int v_escaped = 0;
        if (v[0] == '"' || v[0] == '\'') {
            q = v[0];
        } else if (v[0] == '\\' && (v[1] == '"' || v[1] == '\'')) {
            q = v[1];
            v_escaped = 1;
        }
        if (q) {
            /* Try to locate closing quote in this and following argv tokens */
            size_t cap = 0;
            size_t len = 0;
            char *buf = NULL;

            #define APPCH(ch) do { \
                if (len + 1 >= cap) { \
                    cap = cap ? cap * 2 : 64; \
                    char *nbuf = realloc(buf, cap); \
                    if (!nbuf) { free(buf); goto nojoin; } \
                    buf = nbuf; \
                } \
                buf[len++] = (ch); \
            } while (0)

            const char *p = v + (v_escaped ? 2 : 1); /* skip opening quote */
            int closed = 0;

            for (;;) {
                for (; *p; p++) {
                    if (*p == '\\' && p[1]) {
                        /* keep escaped char as-is */
                        APPCH(p[1]);
                        p++;
                        continue;
                    }
                    if (*p == q) {
                        closed = 1;
                        p++;
                        break;
                    }
                    APPCH(*p);
                }
                if (closed)
                    break;

                /* need next argv token */
                if (i + 1 >= argc)
                    break;
                i++;
                APPCH(' ');
                p = argv[i];
            }

            APPCH('\0');

            args[n].val_alloc = buf;
            args[n].val = args[n].val_alloc;

            #undef APPCH
        }
nojoin:
        n++;
    }

    *out_args = args;
    *out_count = n;
    return 0;
}

static void free_kv_args(kv_arg_t *args, int n) {
    for (int i = 0; i < n; i++) {
        free(args[i].key);
        free(args[i].val_alloc);
    }
    free(args);
}

static long long monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int try_lock_with_timeout(int fd, int timeout_ms) {
    long long start = monotonic_ms();
    for (;;) {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) return 0;
        if (errno != EWOULDBLOCK && errno != EAGAIN) return -1;

        long long now = monotonic_ms();
        if (timeout_ms >= 0 && (now - start) >= timeout_ms) return 1; // timeout

        struct timespec req = { .tv_sec = 0, .tv_nsec = 100 * 1000 * 1000 }; // 100ms
        (void)nanosleep(&req, NULL);
    }
}

static int run_sh_syntax_check(const char *path) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            (void)dup2(devnull, STDERR_FILENO);
            (void)close(devnull);
        }
        execl("/bin/sh", "sh", "-n", path, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

int update_rc_config(const char *filename, int argc, char *argv[]) {
    char temp_path[PATH_MAX];
    snprintf(temp_path, sizeof(temp_path), "%s.tmpXXXXXX", filename);

    char lock_path[PATH_MAX];
    snprintf(lock_path, sizeof(lock_path), "%s.lock", filename);
    int lock_fd = open(lock_path, O_CREAT | O_RDWR, 0600);
    if (lock_fd < 0) return -1;
    int lock_rc = try_lock_with_timeout(lock_fd, 2000);
    if (lock_rc < 0) { close(lock_fd); return -1; }
    if (lock_rc == 1) { close(lock_fd); lock_fd = -1; } // timeout -> force

    struct stat st;
    int have_st = (stat(filename, &st) == 0);

    int fd = mkstemp(temp_path);
    if (fd == -1) {
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -1;
    }

    if (have_st) {
        (void)fchmod(fd, st.st_mode);
        (void)fchown(fd, st.st_uid, st.st_gid);
    }

    FILE *tmp_fp = fdopen(fd, "w");
    if (!tmp_fp) {
        close(fd);
        unlink(temp_path);
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -1;
    }

    kv_arg_t *args = NULL;
    int n_args = 0;
    if (parse_kv_args(argc, argv, &args, &n_args) != 0) {
        fclose(tmp_fp);
        unlink(temp_path);
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -1;
    }

    FILE *src_fp = fopen(filename, "r");
    if (src_fp) {
        char line[2048];
        while (fgets(line, sizeof(line), src_fp)) {
            int replaced = 0;
            for (int i = 0; i < n_args; i++) {
                size_t key_len = strlen(args[i].key);
                const char *p = line;
                p = skip_ws(p);
                if (strncmp(p, args[i].key, key_len) == 0) {
                    p += key_len;
                    p = skip_ws(p);
                    if (*p != '=')
                        continue;
                    p++; /* after '=' */
                    if (args[i].append) {
                        char old_val[1024];
                        char esc_old[2048];
                        char esc_new[2048];
                        extract_existing_value_after_eq(p, old_val, sizeof(old_val));
                        escape_for_double_quotes(old_val, esc_old, sizeof(esc_old));
                        escape_for_double_quotes(args[i].val, esc_new, sizeof(esc_new));
                        if (esc_old[0] != '\0')
                            fprintf(tmp_fp, "%s=\"%s %s\"\n", args[i].key, esc_old, esc_new);
                        else
                            fprintf(tmp_fp, "%s=\"%s\"\n", args[i].key, esc_new);
                    } else {
                        char esc_new[2048];
                        escape_for_double_quotes(args[i].val, esc_new, sizeof(esc_new));
                        fprintf(tmp_fp, "%s=\"%s\"\n", args[i].key, esc_new);
                    }
                    args[i].applied = 1;
                    replaced = 1;
                    break;
                }
            }
            if (!replaced) fputs(line, tmp_fp);
        }
        fclose(src_fp);
    }

    // add new
    for (int i = 0; i < n_args; i++) {
        if (!args[i].applied) {
            char esc_new[2048];
            escape_for_double_quotes(args[i].val, esc_new, sizeof(esc_new));
            fprintf(tmp_fp, "%s=\"%s\"\n", args[i].key, esc_new);
        }
    }

    fclose(tmp_fp);
    free_kv_args(args, n_args);

    int sh_rc = run_sh_syntax_check(temp_path);
    if (sh_rc != 0) {
        fprintf(stderr, "updateconf error: wrong syntax.\n");
        unlink(temp_path);
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -2;
    }

    int rc = rename(temp_path, filename);
    if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
    return rc;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> param=value param2+=append ...\n", argv[0]);
        return 1;
    }
    if (update_rc_config(argv[1], argc - 2, argv + 2) == 0) {
        return 0;
    } else {
        fprintf(stderr, "updateconf: update error\n");
        return 1;
    }
}
