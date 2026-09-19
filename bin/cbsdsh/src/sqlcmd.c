#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "shell.h"
#include "main.h"
#include "nodes.h" /* for other headers */
#include "eval.h"
#include "jobs.h"
#include "show.h"
#include "options.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "trap.h"
#include "mystring.h"
#include "builtins.h"
#include "myhistedit.h"
#include "var.h"
#include <stdbool.h>

#include "sqlcmd.h"

#include <jv.h>
#include <sys/socket.h>
#include <sys/un.h>

#define CBSD_SQLITE_BUSY_TIMEOUT 25000

char *delim;

/*
 * SQL backend selection point (future work):
 * this is the right place to route requests to different DB backends
 * (sqlite/pgsql/mysql/influxdb/...) using backend-specific code.
 *
 * For now we isolate sqlite3 work in a child process to avoid heap
 * corruption in the main shell process on some platforms/libc builds.
 */

static int write_full(int fd, const void *buf, size_t len)
{
	const char *p = (const char *)buf;
	while (len) {
		ssize_t n = write(fd, p, len);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return 0;
		p += (size_t)n;
		len -= (size_t)n;
	}
	return 1;
}

static int read_full(int fd, void *buf, size_t len)
{
	char *p = (char *)buf;
	while (len) {
		ssize_t n = read(fd, p, len);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return 0;
		p += (size_t)n;
		len -= (size_t)n;
	}
	return 1;
}

static int
wait_child_exit_status(pid_t pid)
{
	int st;
	pid_t r;

	for (;;) {
		r = waitpid(pid, &st, 0);
		if (r == pid)
			break;
		if (r < 0 && errno == EINTR)
			continue;
		return 1;
	}
	return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

static int fork_sql_child_stdout(int *out_read_fd)
{
	int pfd[2];
	pid_t pid;

	if (pipe(pfd) != 0)
		return -1;

	pid = fork();
	if (pid < 0) {
		close(pfd[0]);
		close(pfd[1]);
		return -1;
	}

	if (pid == 0) {
		/* child: stdout -> pipe */
		(void)dup2(pfd[1], 1);
		close(pfd[0]);
		close(pfd[1]);
		return 0;
	}

	/* parent */
	close(pfd[1]);
	*out_read_fd = pfd[0];
	return (int)pid;
}

char *
nm(void)
{
	return "cbsdsql";
}

static sqlite3 *
sql_open(const char *dbarg, int open_flags)
{
	char *dbdir;
	const char *dbfile = dbarg;
	char *dbfilebuf = NULL;
	sqlite3 *db = NULL;

	if (dbarg == NULL)
		return NULL;

#ifdef SQLITE_OPEN_SHAREDCACHE
	open_flags &= ~SQLITE_OPEN_SHAREDCACHE;
#endif
#ifdef SQLITE_OPEN_PRIVATECACHE
	open_flags |= SQLITE_OPEN_PRIVATECACHE;
#endif

	if (dbarg[0] != '/') {
		dbdir = lookupvar("dbdir");
		if (!dbdir) {
			out1fmt("dbdir not set!\n");
			return NULL;
		}
		size_t fnlen = strlen(dbdir) + strlen(dbarg) + strlen(DBPOSTFIX) + 2;
		dbfilebuf = calloc(fnlen, sizeof(char));
		if (dbfilebuf == NULL) {
			out1fmt("Out of memory!\n");
			return NULL;
		}
		if (snprintf(dbfilebuf, fnlen, "%s/%s%s", dbdir, dbarg, DBPOSTFIX) < 0) {
			out1fmt("Failed to compose database file name!\n");
			free(dbfilebuf);
			return NULL;
		}
		dbfile = dbfilebuf;
	}

	if (SQLITE_OK != sqlite3_open_v2(dbfile, &db, open_flags, NULL)) {
		out1fmt("%s: Can't open database file: %s\n", nm(), dbfile);

		if (db)
			sqlite3_close(db);

		db = NULL;
	}

	if (dbfilebuf)
		free(dbfilebuf);

	return db;
}

int
sql_exec(sqlite3 *s, const char *sql, ...)
{
	va_list ap;
	const char *sql_to_exec;
	char *sqlbuf = NULL;
	char *errmsg;
	int ret = 0;

	if (s == NULL)
		return 1;
	if (sql == NULL)
		return 1;

	if (strchr(sql, '%') != NULL) {
		va_start(ap, sql);
		sqlbuf = sqlite3_vmprintf(sql, ap);
		va_end(ap);
		sql_to_exec = sqlbuf;
	} else {
		sql_to_exec = sql;
	}

	if (sqlite3_exec(s, sql_to_exec, NULL, NULL, &errmsg) != SQLITE_OK) {
		ERROR_SQLITE(s, sql_to_exec);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	ret = 1;

cleanup:
	if (sqlbuf != NULL)
		sqlite3_free(sqlbuf);

	return (ret);
}

int
sqlCB(sqlite3_stmt *stmt)
{
	int icol;
	const char *colname;
	int allcol;
	int printheader = 0;
	char *sqlcolnames = NULL;

	if (stmt == NULL)
		return 1;

	sqlcolnames = getenv("sqlcolnames");
	allcol = sqlite3_column_count(stmt);

	if ((printheader) && (sqlcolnames == NULL)) {
		for (icol = 0; icol < allcol; icol++) {
			colname = sqlite3_column_name(stmt, icol);
			if (icol != (allcol - 1))
				out1fmt("%s%s", colname, delim);
			else
				out1fmt("%s\n", colname);
		}
	}
	for (icol = 0; icol < allcol; icol++) {
		if (sqlcolnames)
			out1fmt("%s=\"%s\"\n", sqlite3_column_name(stmt, icol),
			    sqlite3_column_text(stmt, icol));
		else {
			if (icol == (allcol - 1))
				out1fmt("%s\n",
				    sqlite3_column_text(stmt, icol));
			else
				out1fmt("%s%s", sqlite3_column_text(stmt, icol),
				    delim);
		}
	}

	return 0;
}


// Helper function to build SQL query from argv
static char *build_query(int argc, char **argv, int start) {
	size_t len = 0;
	for (int i = start; i < argc; i++)
		len += strlen(argv[i]) + 1;
	if (len == 0)
		return NULL;
	char *query = malloc(len);
	if (!query)
		return NULL;
	char *tmp = query;
	for (int i = start; i < argc; i++) {
		strcpy(tmp, argv[i]);
		tmp += strlen(tmp);
		*tmp = ' ';
		tmp++;
	}
	tmp[-1] = 0;
	return query;
}

/*
 * resolve_db_path: resolve $dbarg to absolute sqlite file path
 * using the same logic as sql_open().
 * Caller must free() the returned buffer.
 * Returns NULL on error.
 */
static char *resolve_db_path(const char *dbarg)
{
	char *dbdir;
	char *fullpath;

	if (dbarg == NULL)
		return NULL;

	if (dbarg[0] == '/') {
		size_t len = strlen(dbarg) + 1;
		fullpath = malloc(len);
		if (!fullpath) return NULL;
		memcpy(fullpath, dbarg, len);
		return fullpath;
	}

	dbdir = lookupvar("dbdir");
	if (!dbdir) {
		out1fmt("dbdir not set!\n");
		return NULL;
	}

	size_t len = strlen(dbdir) + strlen(dbarg) + strlen(DBPOSTFIX) + 2;
	fullpath = malloc(len);
	if (!fullpath) {
		out1fmt("Out of memory!\n");
		return NULL;
	}

	snprintf(fullpath, len, "%s/%s%s", dbdir, dbarg, DBPOSTFIX);
	return fullpath;
}

/*
 * daemon_is_available: check if $workdir/var/run/cbsdd.sock exists.
 * Returns 1 if socket file exists, 0 otherwise.
 */
static int daemon_is_available(void)
{
	char *workdir = lookupvar("workdir");
	char sockpath[1024];

	if (!workdir || !*workdir)
		workdir = "/usr/jails";

	snprintf(sockpath, sizeof(sockpath), "%s/var/run/cbsdd.sock", workdir);
	return (access(sockpath, F_OK) == 0);
}

/*
 * daemon_connect: connect to daemon Unix socket.
 * Returns connected fd >= 0, or -1 on error.
 * On stale socket (connect fails), prints warning message.
 *
 * Does NOT take any locks or affect shell state; the caller
 * must close the returned fd when done.
 */
static int daemon_connect(void)
{
	char *workdir = lookupvar("workdir");
	char sockpath[1024];
	int fd;
	struct sockaddr_un addr;

	if (!workdir || !*workdir)
		workdir = "/usr/jails";

	snprintf(sockpath, sizeof(sockpath), "%s/var/run/cbsdd.sock", workdir);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sockpath);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(fd);
		fprintf(stderr,
		    "cbsdsql: socket %s exists but daemon not running? "
		    "Please remove socket or run daemon.\n", sockpath);
		return -1;
	}

	return fd;
}

/*
 * daemon_read_line: read one line (terminated by \n) from fd.
 * NUL-terminates buf. Returns length on success, -1 on error/timeout.
 */
static int daemon_read_line(int fd, char *buf, size_t maxlen)
{
	size_t pos = 0;

	while (pos < maxlen - 1) {
		char c;
		ssize_t n = read(fd, &c, 1);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return -1;
		if (c == '\n')
			break;
		buf[pos++] = c;
	}
	buf[pos] = '\0';
	return (int)pos;
}

int
sqlitecmdrw(int argc, char **argv)
{
	int outfd = -1;
	int cpid;
	int st;
	char buf[8192];
	ssize_t n;
	char *dbpath;
	char *query;
	int dfd;
	char line[8192];

	if (argc < 3) {
		out1fmt("%s: format: %s <dbfile> <query>\n", nm(), nm());
		return 1;
	}

	/* Daemon path: persistent connection through cbsdd */
	if (daemon_is_available()) {
		dbpath = resolve_db_path(argv[1]);
		if (!dbpath)
			return 1;

		query = build_query(argc, argv, 2);
		if (!query) {
			free(dbpath);
			out1fmt("Failed to build query string!\n");
			return 1;
		}

		dfd = daemon_connect();
		if (dfd < 0) {
			free(dbpath);
			free(query);
			return 1;
		}

		write_full(dfd, "W", 1);
		write_full(dfd, dbpath, strlen(dbpath));
		write_full(dfd, "\n", 1);
		write_full(dfd, query, strlen(query));
		write_full(dfd, "\n", 1);

		st = 0;
		for (;;) {
			if (daemon_read_line(dfd, line, sizeof(line)) < 0) {
				st = 1;
				break;
			}
			if (strcmp(line, "END") == 0)
				break;
			if (strncmp(line, "ERR ", 4) == 0) {
				fprintf(stderr, "%s\n", line + 4);
				st = 1;
			}
		}

		close(dfd);
		free(dbpath);
		free(query);
		return st;
	}

	/* Legacy path: fork-per-query */
	cpid = fork_sql_child_stdout(&outfd);
	if (cpid < 0)
		return 1;
	if (cpid == 0) {
		/* child: do sqlite work in isolated process */
		sqlite3 *db;
		char *query = NULL;
		char *cp;

		if ((cp = lookupvar("sqldelimer")) == NULL)
			delim = DEFSQLDELIMER;
		else
			delim = cp;

		/*
		 * Backend selection point (future): switch by env/var here.
		 * Default backend: sqlite.
		 */
		db = sql_open(argv[1],
		    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE);
		if (db == NULL)
			_exit(1);

		sql_exec(db, "PRAGMA mmap_size = 209715200;");
		sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
		sql_exec(db, "PRAGMA journal_mode = WAL;");
		sql_exec(db, "PRAGMA synchronous = NORMAL;");

		query = build_query(argc, argv, 2);
		if (!query) {
			sqlite3_close(db);
			out1fmt("Failed to build query string!\n");
			_exit(1);
		}

		/* Execute arbitrary SQL string, allowing multiple statements separated by ';' */
		if (!sql_exec(db, "BEGIN TRANSACTION;")) {
			free(query);
			sqlite3_close(db);
			_exit(1);
		}

		if (!sql_exec(db, "%s", query)) {
			/* Rollback on error */
			sql_exec(db, "ROLLBACK;");
			free(query);
			sqlite3_close(db);
			_exit(1);
		}

		if (!sql_exec(db, "COMMIT;")) {
			free(query);
			sqlite3_close(db);
			_exit(1);
		}

		free(query);
		sqlite3_close(db);
		/* Ensure buffered shell output is written into the pipe. */
		flushall();
		_exit(0);
	}

	/* parent: stream child's stdout to ours */
	while ((n = read(outfd, buf, sizeof(buf))) > 0)
		out1mem(buf, (size_t)n);
	close(outfd);
	st = wait_child_exit_status((pid_t)cpid);
	/* Ensure buffered output is flushed to the actual stdout. */
	flushout(&output);
	return st;
}

int
sqlitecmdro(int argc, char **argv)
{
	int outfd = -1;
	int cpid;
	int st;
	char buf[8192];
	ssize_t n;
	char *dbpath;
	char *query;
	int dfd;
	char line[8192];

	if (argc < 3) {
		out1fmt("%s: format: %s <dbfile> <query>\n", nm(), nm());
		return 0;
	}

	/* Daemon path: persistent connection through cbsdd */
	if (daemon_is_available()) {
		dbpath = resolve_db_path(argv[1]);
		if (!dbpath)
			return 1;

		query = build_query(argc, argv, 2);
		if (!query) {
			free(dbpath);
			out1fmt("Failed to build query string!\n");
			return 1;
		}

		dfd = daemon_connect();
		if (dfd < 0) {
			free(dbpath);
			free(query);
			return 1;
		}

		write_full(dfd, "R", 1);
		write_full(dfd, dbpath, strlen(dbpath));
		write_full(dfd, "\n", 1);
		write_full(dfd, query, strlen(query));
		write_full(dfd, "\n", 1);

		st = 0;
		for (;;) {
			if (daemon_read_line(dfd, line, sizeof(line)) < 0) {
				st = 1;
				break;
			}
			if (strcmp(line, "END") == 0)
				break;
			if (strncmp(line, "ERR ", 4) == 0) {
				fprintf(stderr, "%s\n", line + 4);
				st = 1;
				break;
			}
			out1mem(line, strlen(line));
			out1mem("\n", 1);
		}

		close(dfd);
		free(dbpath);
		free(query);
		flushout(&output);
		return st;
	}

	/* Legacy path: fork-per-query */
	cpid = fork_sql_child_stdout(&outfd);
	if (cpid < 0)
		return 1;
	if (cpid == 0) {
		/* child: do sqlite work in isolated process */
		sqlite3 *db;
		char *query = NULL;
		int ret = 0;
		sqlite3_stmt *stmt = NULL;
		char *cp;
		int maxretry = 50;
		int retry = 0;

		if ((cp = lookupvar("sqldelimer")) == NULL)
			delim = DEFSQLDELIMER;
		else
			delim = cp;

		/*
		 * Backend selection point (future): switch by env/var here.
		 * Default backend: sqlite.
		 */
		db = sql_open(argv[1], SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE);
		if (db == NULL)
			_exit(1);

		sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
		sql_exec(db, "PRAGMA mmap_size = 209715200;");

		query = build_query(argc, argv, 2);
		if (!query) {
			sqlite3_close(db);
			out1fmt("Failed to build query string!\n");
			_exit(1);
		}

		do {
			ret = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
			if (ret == SQLITE_OK)
				break;
			if (stmt) {
				sqlite3_finalize(stmt);
				stmt = NULL;
			}
			retry++;
			if (retry > maxretry)
				break;
		} while (ret != SQLITE_OK);

		if (ret == SQLITE_OK) {
			ret = sqlite3_step(stmt);
			while (ret == SQLITE_ROW) {
				sqlCB(stmt);
				ret = sqlite3_step(stmt);
			}
		}

		if (stmt)
			sqlite3_finalize(stmt);
		free(query);
		sqlite3_close(db);
		/* Ensure buffered shell output is written into the pipe. */
		flushall();
		_exit(0);
	}

	/* parent: stream child's stdout to ours */
	while ((n = read(outfd, buf, sizeof(buf))) > 0)
		out1mem(buf, (size_t)n);
	close(outfd);
	st = wait_child_exit_status((pid_t)cpid);
	/* Ensure buffered output is flushed to the actual stdout. */
	flushout(&output);
	return st;
}

/*
 * sqlitecmdquery:
 *   Read-only query against dbfile and print one JSON per row to stdout.
 *
 * Usage (in shell):
 *   cbsdsqlquery <dbfile> "<query>"
 */
int
sqlitecmdquery(int argc, char **argv)
{
	int outfd = -1;
	int cpid;
	int st;
	char buf[8192];
	ssize_t n;
	char *dbpath;
	char *query;
	int dfd;
	char line[8192];

	if (argc != 3) {
		out1fmt("usage: cbsdsqlquery <dbfile> <query>\n");
		return 1;
	}

	/* Daemon path: persistent connection through cbsdd */
	if (daemon_is_available()) {
		dbpath = resolve_db_path(argv[1]);
		if (!dbpath)
			return 1;

		query = malloc(strlen(argv[2]) + 1);
		if (!query) {
			free(dbpath);
			out1fmt("Out of memory!\n");
			return 1;
		}
		strcpy(query, argv[2]);

		dfd = daemon_connect();
		if (dfd < 0) {
			free(dbpath);
			free(query);
			return 1;
		}

		write_full(dfd, "J", 1);
		write_full(dfd, dbpath, strlen(dbpath));
		write_full(dfd, "\n", 1);
		write_full(dfd, query, strlen(query));
		write_full(dfd, "\n", 1);

		st = 0;
		for (;;) {
			if (daemon_read_line(dfd, line, sizeof(line)) < 0) {
				st = 1;
				break;
			}
			if (strcmp(line, "END") == 0)
				break;
			if (strncmp(line, "ERR ", 4) == 0) {
				fprintf(stderr, "%s\n", line + 4);
				st = 1;
				break;
			}
			out1mem(line, strlen(line));
			out1mem("\n", 1);
		}

		close(dfd);
		free(dbpath);
		free(query);
		flushout(&output);
		return st;
	}

	/* Legacy path: fork-per-query */
	cpid = fork_sql_child_stdout(&outfd);
	if (cpid < 0)
		return 1;
	if (cpid == 0) {
		/* child: do sqlite work in isolated process */
		sqlite3 *db;
		int ret = 0;
		sqlite3_stmt *stmt = NULL;
		int maxretry = 50;
		int retry = 0;

		/*
		 * Backend selection point (future): switch by env/var here.
		 * Default backend: sqlite.
		 */
		db = sql_open(argv[1], SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE);
		if (db == NULL)
			_exit(1);

		sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
		sql_exec(db, "PRAGMA mmap_size = 209715200;");

		do {
			ret = sqlite3_prepare_v2(db, argv[2], -1, &stmt, NULL);
			if (ret == SQLITE_OK)
				break;
			if (stmt) {
				sqlite3_finalize(stmt);
				stmt = NULL;
			}
			retry++;
			if (retry > maxretry)
				break;
		} while (ret != SQLITE_OK);

		if (ret == SQLITE_OK) {
			ret = sqlite3_step(stmt);
			while (ret == SQLITE_ROW) {
				int icol;
				int allcol = sqlite3_column_count(stmt);
				jv row = jv_object();

				for (icol = 0; icol < allcol; icol++) {
					const char *colname = sqlite3_column_name(stmt, icol);
					switch (sqlite3_column_type(stmt, icol)) {
					case SQLITE_INTEGER:
						row = jv_object_set(row, jv_string(colname),
						    jv_number((double)sqlite3_column_int64(stmt, icol)));
						break;
					case SQLITE_FLOAT:
						row = jv_object_set(row, jv_string(colname),
						    jv_number(sqlite3_column_double(stmt, icol)));
						break;
					case SQLITE_TEXT: {
						const unsigned char *txt_uc = sqlite3_column_text(stmt, icol);
						const char *txt = txt_uc ? (const char *)txt_uc : "";
						row = jv_object_set(row, jv_string(colname), jv_string(txt));
						break;
					}
					case SQLITE_NULL:
						row = jv_object_set(row, jv_string(colname), jv_null());
						break;
					case SQLITE_BLOB:
					default:
						row = jv_object_set(row, jv_string(colname), jv_null());
						break;
					}
				}
				jv dumped = jv_dump_string(row, 0);
				if (!jv_is_valid(dumped)) {
					jv msg = jv_invalid_get_msg(jv_copy(dumped));
					out1fmt("%s\n", jv_string_value(msg));
					jv_free(msg);
					jv_free(dumped);
					jv_free(row);
					ret = 1;
					break;
				}
				out1fmt("%s\n", jv_string_value(dumped));
				jv_free(dumped);
				jv_free(row);
				ret = sqlite3_step(stmt);
			}
		}

		if (stmt)
			sqlite3_finalize(stmt);
		sqlite3_close(db);

		/* Ensure buffered shell output is written into the pipe. */
		flushall();
		_exit(0);
	}

	/* parent: stream child's stdout to ours */
	while ((n = read(outfd, buf, sizeof(buf))) > 0)
		out1mem(buf, (size_t)n);
	close(outfd);
	st = wait_child_exit_status((pid_t)cpid);
	/* Ensure buffered output is flushed to the actual stdout. */
	flushout(&output);
	return st;
}

/*
 * sqlitecmdro_vars:
 *   Read-only query like sqlitecmdro, but instead of printing result,
 *   assign columns into shell variables. If the query returns multiple
 *   rows, values from each column are concatenated into the corresponding
 *   variable separated by '\n'.
 *
 * Usage (in shell):
 *   cbsdsqlro_vars <dbfile> <query> [ var1 ...]
 *
 *  Set of returned variables - matches columns selected ("*" -> all table columns)
 *  You may specify variables names explicitly as argv or let them to be named after columns
 *  If the number of specified variables is less than the number of columns in the query,
 *  column names will be used for rest of variables.
 *
 * Example:
 *   cbsdsqlro_vars "${_sqlfile}" "SELECT mnt_start,mnt_stop FROM bhyve WHERE jname='$1'"
 *   cbsdsqlro_vars "${_sqlfile}" "SELECT jid,jname FROM bhyve WHERE jname='$1'" myjid myjname
 */
int
sqlitecmdro_vars(int argc, char **argv)
{
	int pfd[2];
	pid_t pid;
	int st;
	int nvars;
	int i;
	char *dbpath;
	char *query;
	int dfd;

	/* Need at least: dbfile query var1 */
	if (argc < 3) {
		out1fmt("usage: cbsdsqlro_vars <dbfile> <query> [ var1 [var2 ...]]\n");
		return 1;
	}

	nvars = argc - 3;

	/* Daemon path: persistent connection through cbsdd */
	if (daemon_is_available()) {
		int ncols = 0;
		char colnames[64][128];
		char **colvals = NULL;   /* per-column accumulated values */
		size_t *collens = NULL;
		int got_row = 0;

		dbpath = resolve_db_path(argv[1]);
		if (!dbpath)
			return 1;

		query = malloc(strlen(argv[2]) + 1);
		if (!query) {
			free(dbpath);
			return 1;
		}
		strcpy(query, argv[2]);

		dfd = daemon_connect();
		if (dfd < 0) {
			free(dbpath);
			free(query);
			return 1;
		}

		write_full(dfd, "V", 1);
		write_full(dfd, dbpath, strlen(dbpath));
		write_full(dfd, "\n", 1);
		write_full(dfd, query, strlen(query));
		write_full(dfd, "\n", 1);

		/* First line: C:col1,col2,col3 */
		char line[8192];
		int n = daemon_read_line(dfd, line, sizeof(line));
		if (n < 0 || strncmp(line, "C:", 2) != 0 || strncmp(line, "ERR ", 4) == 0) {
			fprintf(stderr, "%s\n", line);
			close(dfd);
			free(dbpath);
			free(query);
			return 1;
		}

		/* Parse column names */
		char *p = line + 2;
		while (ncols < 64 && *p) {
			char *comma = strchr(p, ',');
			if (comma) {
				size_t len = (size_t)(comma - p);
				if (len >= sizeof(colnames[0])) len = sizeof(colnames[0]) - 1;
				memcpy(colnames[ncols], p, len);
				colnames[ncols][len] = '\0';
				p = comma + 1;
			} else {
				snprintf(colnames[ncols], sizeof(colnames[ncols]), "%s", p);
				p += strlen(p);
			}
			ncols++;
		}

		if (ncols == 0) {
			close(dfd);
			free(dbpath);
			free(query);
			return 1;
		}

		/* Allocate column value buffers */
		colvals = calloc((size_t)ncols, sizeof(char *));
		collens = calloc((size_t)ncols, sizeof(size_t));
		if (!colvals || !collens) {
			close(dfd);
			free(dbpath);
			free(query);
			free(colvals);
			free(collens);
			return 1;
		}

		/* Read rows until END */
		for (;;) {
			n = daemon_read_line(dfd, line, sizeof(line));
			if (n < 0)
				break;
			if (strcmp(line, "END") == 0)
				break;
			if (strncmp(line, "ERR ", 4) == 0)
				break;

			got_row = 1;

			/* Parse pipe-delimited values */
			char *pos = line;
			for (int j = 0; j < ncols; j++) {
				char *pipe = strchr(pos, '|');
				char val[4096];
				if (pipe) {
					size_t vlen = (size_t)(pipe - pos);
					if (vlen >= sizeof(val)) vlen = sizeof(val) - 1;
					memcpy(val, pos, vlen);
					val[vlen] = '\0';
					pos = pipe + 1;
				} else {
					snprintf(val, sizeof(val), "%s", pos);
					pos += strlen(pos);
				}

				size_t vlen = strlen(val);
				size_t need = collens[j] + vlen + (collens[j] ? 1 : 0);
				char *nv = realloc(colvals[j], need + 1);
				if (!nv) continue;
				colvals[j] = nv;
				if (collens[j]) {
					colvals[j][collens[j]] = '\n';
					memcpy(colvals[j] + collens[j] + 1, val, vlen);
					collens[j] = need;
				} else {
					memcpy(colvals[j], val, vlen);
					collens[j] = vlen;
				}
				colvals[j][collens[j]] = '\0';
			}
		}

		close(dfd);
		free(dbpath);
		free(query);

		if (!got_row) {
			for (i = 0; i < nvars; i++)
				setvar(argv[3 + i], "", 0);
			free(colvals);
			free(collens);
			return 1;
		}

		/* Set shell variables */
		for (int j = 0; j < ncols; j++) {
			const char *varname = (j < nvars) ? argv[3 + j] : colnames[j];
			setvar(varname, colvals[j] ? colvals[j] : "", 0);
		}

		for (int j = 0; j < ncols; j++)
			free(colvals[j]);
		free(colvals);
		free(collens);
		return 0;
	}

	/* Legacy path: fork-per-query */

	if (pipe(pfd) != 0)
		return 1;

	pid = fork();
	if (pid < 0) {
		close(pfd[0]);
		close(pfd[1]);
		return 1;
	}

	if (pid == 0) {
		/* child: execute query and send framed results to parent */
		sqlite3 *db;
		sqlite3_stmt *stmt = NULL;
		int ret = 0;
		int maxretry = 50;
		int retry = 0;
		int got_row = 0;

		close(pfd[0]);

		/*
		 * Backend selection point (future): switch by env/var here.
		 * Default backend: sqlite.
		 */
		db = sql_open(argv[1], SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE);
		if (db == NULL)
			_exit(1);

		sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
		sql_exec(db, "PRAGMA mmap_size = 209715200;");

		do {
			ret = sqlite3_prepare_v2(db, argv[2], -1, &stmt, NULL);
			if (ret == SQLITE_OK)
				break;
			if (stmt) {
				sqlite3_finalize(stmt);
				stmt = NULL;
			}
			retry++;
			if (retry > maxretry)
				break;
		} while (ret != SQLITE_OK);

		if (ret != SQLITE_OK) {
			if (stmt)
				sqlite3_finalize(stmt);
			sqlite3_close(db);
			_exit(1);
		}

		/* dynamic arrays for values per column/var */
		int ncols = sqlite3_column_count(stmt);
		char **names = calloc((size_t)ncols, sizeof(char *));
		char **vals = calloc((size_t)ncols, sizeof(char *));
		size_t *vlen = calloc((size_t)ncols, sizeof(size_t));
		if (!names || !vals || !vlen) {
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			_exit(1);
		}

		for (int j = 0; j < ncols; j++) {
			const char *colname = sqlite3_column_name(stmt, j);
			const char *varname = (j < nvars) ? argv[3 + j] : colname;
			names[j] = strdup(varname);
			if (!names[j]) {
				sqlite3_finalize(stmt);
				sqlite3_close(db);
				_exit(1);
			}
		}

		ret = sqlite3_step(stmt);
		while (ret == SQLITE_ROW) {
			got_row = 1;
			for (int j = 0; j < ncols; j++) {
				const unsigned char *txt_uc = sqlite3_column_text(stmt, j);
				const char *txt = txt_uc ? (const char *)txt_uc : "";
				size_t sl = strlen(txt);
				size_t need = vlen[j] + sl + (vlen[j] ? 1 : 0);
				char *nv = realloc(vals[j], need + 1);
				if (!nv) {
					sqlite3_finalize(stmt);
					sqlite3_close(db);
					_exit(1);
				}
				vals[j] = nv;
				if (vlen[j]) {
					vals[j][vlen[j]] = '\n';
					memcpy(vals[j] + vlen[j] + 1, txt, sl);
					vlen[j] = need;
				} else {
					memcpy(vals[j], txt, sl);
					vlen[j] = sl;
				}
				vals[j][vlen[j]] = '\0';
			}
			ret = sqlite3_step(stmt);
		}

		sqlite3_finalize(stmt);
		sqlite3_close(db);

		if (!got_row) {
			/* indicate "no rows" */
			close(pfd[1]);
			_exit(1);
		}

		/* frame: uint32 count, then (uint32 nlen, uint32 vlen, bytes...) */
		uint32_t cnt = (uint32_t)ncols;
		if (!write_full(pfd[1], &cnt, sizeof(cnt)))
			_exit(1);
		for (int j = 0; j < ncols; j++) {
			uint32_t nl = (uint32_t)strlen(names[j]);
			uint32_t vl = (uint32_t)(vals[j] ? strlen(vals[j]) : 0);
			if (!write_full(pfd[1], &nl, sizeof(nl)) ||
			    !write_full(pfd[1], &vl, sizeof(vl)) ||
			    !write_full(pfd[1], names[j], nl) ||
			    (vl && !write_full(pfd[1], vals[j], vl))) {
				_exit(1);
			}
		}
		close(pfd[1]);
		_exit(0);
	}

	/* parent: read framed results and set vars */
	close(pfd[1]);
	uint32_t cnt = 0;
	if (!read_full(pfd[0], &cnt, sizeof(cnt))) {
		close(pfd[0]);
		(void)wait_child_exit_status(pid);
		for (i = 0; i < nvars; i++)
			setvar(argv[3 + i], "", 0);
		return 1;
	}
	for (uint32_t j = 0; j < cnt; j++) {
		uint32_t nl = 0, vl = 0;
		char *nmbuf = NULL;
		char *vlbuf = NULL;
		if (!read_full(pfd[0], &nl, sizeof(nl)) ||
		    !read_full(pfd[0], &vl, sizeof(vl))) {
			close(pfd[0]);
			(void)wait_child_exit_status(pid);
			return 1;
		}
		nmbuf = ckmalloc((size_t)nl + 1);
		if (!read_full(pfd[0], nmbuf, nl)) {
			ckfree(nmbuf);
			close(pfd[0]);
			(void)wait_child_exit_status(pid);
			return 1;
		}
		nmbuf[nl] = '\0';
		vlbuf = ckmalloc((size_t)vl + 1);
		if (vl) {
			if (!read_full(pfd[0], vlbuf, vl)) {
				ckfree(nmbuf);
				ckfree(vlbuf);
				close(pfd[0]);
				(void)wait_child_exit_status(pid);
				return 1;
			}
		}
		vlbuf[vl] = '\0';
		setvar(nmbuf, vlbuf, 0);
		ckfree(nmbuf);
		ckfree(vlbuf);
	}
	close(pfd[0]);
	st = wait_child_exit_status(pid);
	return st;
}

int
update_idlecmd(int argc, char **argv)
{
	sqlite3 *db;

	if (argc != 2) {
		out1fmt("%d, usage: update_idle <nodename>\n", argc);
		return 0;
	}

	db = sql_open("nodes",
	    SQLITE_OPEN_READWRITE | SQLITE_OPEN_SHAREDCACHE);
	if (db == NULL)
		return 1;

	sqlite3_busy_timeout(db, CBSD_SQLITE_BUSY_TIMEOUT);
	sql_exec(db, "PRAGMA mmap_size = 209715200;");
	sql_exec(db, "PRAGMA journal_mode = WAL;");
	sql_exec(db, "PRAGMA synchronous = NORMAL;");
	sql_exec(db, "UPDATE nodelist SET idle=datetime('now','localtime') WHERE nodename='%q'",
	    argv[1]);
	sqlite3_close(db);

	return 0;
}
