// Part of the CBSD Project
// Exec cmd via jexec with clean environment
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
// a shell exit is reported as 0, an execv failure is not.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ENV_PATH_LEN	512
#define ELF_CMD_LEN	512
#define ELF_BUF_LEN	128

// FreeBSD 14.3+ supports jexec -d (homedir)
#define FREEBSD_VER_14_3	1403000

// Sentinel: distinguish execv failure from shell exit code.
// Used only when cmd==NULL (interactive login) to decide whether
// the non-zero exit came from execv failure or from the user's shell.
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

static int
get_freebsd_ver(const char *cix_distdir)
{
	char elf_cmd[ELF_CMD_LEN];
	char buffer[ELF_BUF_LEN];

	snprintf(elf_cmd, sizeof(elf_cmd),
	    "%s/misc/elf_tables --ver /bin/sh", cix_distdir);

	FILE *fp = popen(elf_cmd, "r");
	if (fp == NULL) {
		fprintf(stderr, "jexec_env: popen failed: %s\n", elf_cmd);
		return 0;
	}

	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		pclose(fp);
		return 0;
	}
	pclose(fp);

	return atoi(buffer);
}

static char **
build_jexec_argv(char *jname, char *user, char *homedir, char **cmd,
    int use_user, int use_homedir)
{
	int ncmd = 0;
	int i, n;
	char **av;

	if (cmd != NULL)
		for (; cmd[ncmd] != NULL; ncmd++)
			;
	n = 1 + (use_user ? 2 : 0) + (use_homedir ? 2 : 0) + 1 + ncmd + 1;
	av = malloc(sizeof(char *) * (size_t)n);
	if (av == NULL)
		return NULL;
	i = 0;
	av[i++] = "jexec";
	if (use_user) {
		av[i++] = "-U";
		av[i++] = user;
	}
	if (use_homedir) {
		av[i++] = "-d";
		av[i++] = homedir;
	}
	av[i++] = jname;
	for (n = 0; n < ncmd; n++)
		av[i++] = cmd[n];
	av[i] = NULL;
	return av;
}

/* cd inside the jail, then exec the command. Used when jexec has no -d. */
static char **
cd_wrap(char *shell, char *dir, char **cmd)
{
	static const char script[] = "cd \"$1\" && shift && exec \"$@\"";
	int ncmd = 0;
	int i;
	char **av;

	if (cmd != NULL)
		while (cmd[ncmd] != NULL)
			ncmd++;
	av = malloc(sizeof(char *) * (size_t)(6 + (ncmd > 0 ? ncmd : 1)));
	if (av == NULL)
		return NULL;
	av[0] = shell;
	av[1] = "-c";
	av[2] = (char *)script;
	av[3] = "_";
	av[4] = dir;
	if (ncmd == 0) {
		av[5] = shell;
		av[6] = NULL;
	} else {
		for (i = 0; i < ncmd; i++)
			av[5 + i] = cmd[i];
		av[5 + ncmd] = NULL;
	}
	return av;
}

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
	const char *cix_distdir_env = getenv("CIX_DISTDIR");
	const char *cix_distdir;
	int home_set = 0;
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

	cix_distdir = (cix_distdir_env != NULL) ? cix_distdir_env : "/usr/local/cbsd";

	const char *term = getenv("TERM");
	const char *blocksize = getenv("BLOCKSIZE");

	use_user = (o->user != NULL && strcmp(o->user, "root") != 0);

	if (o->dir != NULL && o->dir[0] != '\0') {
		int freebsd_ver = get_freebsd_ver(cix_distdir);
		if (freebsd_ver >= FREEBSD_VER_14_3)
			home_set = 1;
	}

	pid_t pid = fork();

	if (pid == 0) {
		extern char **environ;
		char *cleanenv[1] = { NULL };
		char *sh_only[2];
		char **run;
		char **jexec_argv;

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

		if (!home_set && o->dir != NULL && o->dir[0] != '\0') {
			run = cd_wrap(o->shell_bin, o->dir, run);
			if (run == NULL)
				_exit(1);
		}

		jexec_argv = build_jexec_argv(o->jname, o->user, o->dir, run,
		    use_user, home_set);
		if (jexec_argv == NULL)
			_exit(1);

		execv("/usr/sbin/jexec", jexec_argv);
		perror("jexec_env: execv failed");
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
