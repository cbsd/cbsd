// Part of the CBSD Project
// Enter a jail and exec a command with a clean environment.
//
// jexec_env [-l] [-U user] [-d dir] [-e envfile] [-s no|auto|/path] jname [cmd arg ...]
// Flags, then the jail name, then the command. -s selects the shell:
//   no    exec the command as given
//   auto  if the first argument (the script, cmd) contains metacharacters,
//         run /bin/sh -c <cmd> <params...>; otherwise exec the arguments
//   /path /path -c <cmd> <params...> when a command is present
// Arguments are not joined. The first is the script, the rest are its
// parameters. No command runs the shell so it can read stdin.
// -l is an interactive login:
// a shell exit is reported as 0, an exec failure is not.
//
// Environment files live on the host and are read before jail_attach.
// The directory (-d) is a path inside the jail, so chdir happens after
// attach, while the process is still root. -U is looked up after attach,
// in the jail's passwd.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/jail.h>
#include <jail.h>
#include <pwd.h>
#include <login_cap.h>
#endif

#define ENV_PATH_LEN	512

// Sentinel: distinguish exec failure from a shell exit.
// With -l, a shell exit is reported as 0 and this code is not.
#define JEXEC_EXECV_FAIL_CODE	2

static void
jname_putenv(const char *path)
{
	FILE *fp;
	char *line = NULL;
	size_t cap = 0;
	ssize_t n;

	if (path == NULL || path[0] == '\0')
		return;
	fp = fopen(path, "r");
	if (fp == NULL)
		return;

	while ((n = getline(&line, &cap, fp)) != -1) {
		while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
			line[--n] = '\0';
		if (line[0] == '\0' || line[0] == '#')
			continue;
		if (strncmp(line, "export ", 7) == 0)
			memmove(line, line + 7, strlen(line + 7) + 1);
		char *eq = strchr(line, '=');
		if (eq == NULL || eq == line)
			continue;
		*eq = '\0';
		setenv(line, eq + 1, 1);
	}
	free(line);
	fclose(fp);
}

#ifdef __FreeBSD__
/* Attach, then chdir. -d is a path inside the jail, so it is resolved here. */
static int
enter_jail(const char *jname, const char *dir)
{
	int jid;

	jid = jail_getid(jname);
	if (jid < 0) {
		fprintf(stderr, "jexec_env: %s\n", jail_errmsg);
		return -1;
	}
	if (jail_attach(jid) < 0) {
		perror("jexec_env: jail_attach");
		return -1;
	}
	/* Attach sets cwd to the jail root. A relative -d starts from there. */
	if (chdir("/") != 0) {
		perror("jexec_env: chdir: /");
		return -1;
	}
	if (dir != NULL && dir[0] != '\0' && chdir(dir) != 0) {
		fprintf(stderr, "jexec_env: chdir: %s: %s\n",
		    dir, strerror(errno));
		return -1;
	}
	return 0;
}

/*
 * Switch to the jail user. Called after attach, so passwd and login.conf
 * are the jail's. Login class supplies limits, umask and priority.
 * LOGIN_SETENV and LOGIN_SETPATH are omitted: they would replace the
 * environment loaded on the host before attach.
 */
static int
drop_privs(const char *user)
{
	struct passwd *pwd;
	login_cap_t *lcap;
	uid_t uid;
	gid_t gid;
	char name[64];
	unsigned int flags;

	pwd = getpwnam(user);
	if (pwd == NULL) {
		fprintf(stderr, "jexec_env: %s: no such user\n", user);
		return -1;
	}
	uid = pwd->pw_uid;
	gid = pwd->pw_gid;
	strlcpy(name, pwd->pw_name, sizeof(name));

	lcap = login_getpwclass(pwd);
	if (lcap != NULL) {
		flags = LOGIN_SETALL & ~LOGIN_SETGROUP & ~LOGIN_SETLOGIN &
		    ~LOGIN_SETUSER & ~LOGIN_SETENV & ~LOGIN_SETPATH;
		if (setusercontext(lcap, pwd, uid, flags) != 0) {
			perror("jexec_env: setusercontext");
			login_close(lcap);
			endpwent();
			return -1;
		}
		login_close(lcap);
	}
	if (initgroups(name, gid) != 0) {
		perror("jexec_env: initgroups");
		endpwent();
		return -1;
	}
	if (setgid(gid) != 0) {
		perror("jexec_env: setgid");
		endpwent();
		return -1;
	}
	if (setuid(uid) != 0) {
		perror("jexec_env: setuid");
		endpwent();
		return -1;
	}
	endpwent();
	return 0;
}
#else
static int
enter_jail(const char *jname, const char *dir)
{
	(void)jname;
	(void)dir;
	fprintf(stderr, "jexec_env: jail entry is supported on FreeBSD only\n");
	return -1;
}

static int
drop_privs(const char *user)
{
	(void)user;
	fprintf(stderr, "jexec_env: jail entry is supported on FreeBSD only\n");
	return -1;
}
#endif

enum {
	SH_NO = 0,
	SH_AUTO,
	SH_BIN
};

struct jopt {
	char *jname;
	char *user;
	char *dir;
	char *envfile;
	char **cmd;
	char *shell_bin;
	int shell_mode;
	int login;
};

static int
shell_meta(const char *s)
{
	/* '=' is not a metacharacter: it stays inside an argument. */
	return s != NULL && strpbrk(s,
	    " \t\n|&;<>()$`\\\"'*?[]{}!~") != NULL;
}

/* 1: run as shell -c. The script is cmd[0]; later arguments are its parameters. */
static int
want_shell(int mode, char **cmd)
{
	if (cmd == NULL || cmd[0] == NULL)
		return 0;
	if (mode == SH_NO)
		return 0;
	if (mode == SH_BIN)
		return 1;
	return shell_meta(cmd[0]);
}

static int
execute_cmd(struct jopt *o)
{
	const char *workdir = getenv("workdir");
	int status = 0;
	int use_user;

	if (workdir == NULL) {
		fprintf(stderr, "jexec_env: 'workdir' is not set\n");
		exit(1);
	}
	if (o->jname == NULL || o->jname[0] == '\0') {
		fprintf(stderr, "jexec_env: jail name is required\n");
		exit(1);
	}

	const char *term = getenv("TERM");
	const char *blocksize = getenv("BLOCKSIZE");

	use_user = (o->user != NULL && strcmp(o->user, "root") != 0);

	pid_t pid = fork();

	if (pid == 0) {
		extern char **environ;
		char *cleanenv[1] = { NULL };
		char *sh_only[2];
		char **run;

		environ = cleanenv;

		if (term != NULL)
			setenv("TERM", term, 1);
		if (blocksize != NULL)
			setenv("BLOCKSIZE", blocksize, 1);
		if (o->dir != NULL && o->dir[0] != '\0')
			setenv("HOME", o->dir, 1);

		char env_path[ENV_PATH_LEN];
		snprintf(env_path, sizeof(env_path),
		    "%s/jails-system/%s/environment", workdir, o->jname);
		jname_putenv(env_path);

		snprintf(env_path, sizeof(env_path),
		    "%s/jails-system/%s/environment.local", workdir, o->jname);
		jname_putenv(env_path);
		jname_putenv(o->envfile);

		if (o->cmd == NULL || o->cmd[0] == NULL) {
			sh_only[0] = o->shell_bin;
			sh_only[1] = NULL;
			run = sh_only;
		} else if (!want_shell(o->shell_mode, o->cmd)) {
			run = o->cmd;
		} else {
			int ncmd = 0;
			int i;

			while (o->cmd[ncmd] != NULL)
				ncmd++;
			run = malloc(sizeof(char *) * (size_t)(ncmd + 3));
			if (run == NULL)
				_exit(1);
			run[0] = o->shell_bin;
			run[1] = "-c";
			for (i = 0; i < ncmd; i++)
				run[i + 2] = o->cmd[i];
			run[ncmd + 2] = NULL;
		}

		/* Env files are host paths. Attach only after they are loaded. */
		if (enter_jail(o->jname, o->dir) != 0)
			_exit(1);
		if (use_user && drop_privs(o->user) != 0)
			_exit(1);

		execvp(run[0], run);
		perror("jexec_env: execvp failed");
		_exit(JEXEC_EXECV_FAIL_CODE);
	} else if (pid > 0) {
		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			int exit_code = WEXITSTATUS(status);

			if (o->login && exit_code != JEXEC_EXECV_FAIL_CODE)
				return 0;
			return exit_code;
		} else if (WIFSIGNALED(status)) {
			return 128 + WTERMSIG(status);
		}
		return 1;
	} else {
		perror("jexec_env: fork failed");
		exit(1);
	}

	return 0;
}

static void
usage(void)
{
	fprintf(stderr,
	    "Usage: jexec_env [-l] [-U user] [-d dir] [-e envfile] [-s no|auto|/path] jname [cmd ...]\n");
}

int
main(int argc, char **argv)
{
	struct jopt o;
	int ch;

	memset(&o, 0, sizeof(o));
	o.user = "root";
	o.shell_mode = SH_AUTO;
	o.shell_bin = "/bin/sh";

	while ((ch = getopt(argc, argv, "U:d:e:s:l")) != -1) {
		switch (ch) {
		case 'U':
			o.user = optarg;
			break;
		case 'd':
			o.dir = optarg;
			break;
		case 'e':
			o.envfile = optarg;
			break;
		case 's':
			if (strcmp(optarg, "no") == 0) {
				o.shell_mode = SH_NO;
				o.shell_bin = "/bin/sh";
			} else if (strcmp(optarg, "auto") == 0) {
				o.shell_mode = SH_AUTO;
				o.shell_bin = "/bin/sh";
			} else if (optarg[0] == '/') {
				o.shell_mode = SH_BIN;
				o.shell_bin = optarg;
			} else {
				fprintf(stderr,
				    "jexec_env: -s must be no, auto, or /path/to/shell\n");
				usage();
				return 1;
			}
			break;
		case 'l':
			o.login = 1;
			break;
		default:
			usage();
			return 1;
		}
	}

	argc -= optind;
	argv += optind;
	if (argc >= 1 && strcmp(argv[0], "--") == 0) {
		argc--;
		argv++;
	}
	if (argc < 1) {
		fprintf(stderr, "jexec_env: jail name is required\n");
		usage();
		return 1;
	}

	o.jname = argv[0];
	if (argc > 1 && strcmp(argv[1], "--") == 0)
		o.cmd = argv + 2;
	else
		o.cmd = argv + 1;
	return execute_cmd(&o);
}
