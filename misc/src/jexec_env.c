// Part of the CBSD Project
// Exec cmd via jexec with clean environment
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE	256
#define MAX_JEXEC_ARGS	16
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
	FILE *file = fopen(path, "r");
	if (file == NULL)
		return;

	char line[MAX_LINE];
	while (fgets(line, sizeof(line), file) != NULL) {
		line[strcspn(line, "\n")] = '\0';
		if (line[0] == '\0' || line[0] == '#')
			continue;
		char *eq = strchr(line, '=');
		if (eq == NULL)
			continue;
		*eq = '\0';
		const char *name = line;
		const char *value = eq + 1;
		setenv(name, value, 1);
	}
	fclose(file);
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

static void
build_jexec_argv(char **jexec_argv, int max_args,
    char *jname, char *user, char *homedir,
    char *shell, char *cmd, int use_user, int use_homedir)
{
	int i = 0;

	jexec_argv[i++] = "jexec";

	if (use_user) {
		jexec_argv[i++] = "-U";
		jexec_argv[i++] = user;
	}

	if (use_homedir) {
		jexec_argv[i++] = "-d";
		jexec_argv[i++] = homedir;
	}

	jexec_argv[i++] = jname;
	jexec_argv[i++] = shell;

	if (cmd != NULL) {
		if (i + 3 >= max_args) {
			fprintf(stderr, "jexec_env: too many jexec arguments\n");
			_exit(1);
		}
		jexec_argv[i++] = "-c";
		jexec_argv[i++] = cmd;
	}

	jexec_argv[i] = NULL;
}

static int
execute_cmd(char *jname, char **argv)
{
	const char *workdir = getenv("workdir");
	const char *cix_distdir_env = getenv("CIX_DISTDIR");
	const char *cix_distdir;
	int home_set = 0;
	int status = 0;

	if (workdir == NULL) {
		fprintf(stderr, "jexec_env: 'workdir' is not set\n");
		exit(1);
	}
	if (jname == NULL) {
		fprintf(stderr, "jexec_env: jail name is required\n");
		exit(1);
	}
	if (argv[2] == NULL || argv[4] == NULL) {
		fprintf(stderr, "jexec_env: insufficient arguments (user shell required)\n");
		exit(1);
	}

	cix_distdir = (cix_distdir_env != NULL) ? cix_distdir_env : "/usr/local/cbsd";

	const char *term = getenv("TERM");
	const char *blocksize = getenv("BLOCKSIZE");
	char *user = argv[2];
	char *homedir = argv[3];
	char *shell = argv[4];
	char *cmd = argv[5];

	// Determine if -d flag should be used (FreeBSD 14.3+)
	if (homedir != NULL) {
		int freebsd_ver = get_freebsd_ver(cix_distdir);
		if (freebsd_ver >= FREEBSD_VER_14_3)
			home_set = 1;
	}

	pid_t pid = fork();

	if (pid == 0) {
		// Child process: clean environment, load jail env, exec jexec
		extern char **environ;
		char *cleanenv[1] = { NULL };
		environ = cleanenv;

		if (term != NULL)
			setenv("TERM", term, 1);
		if (blocksize != NULL)
			setenv("BLOCKSIZE", blocksize, 1);
		if (homedir != NULL)
			setenv("HOME", homedir, 1);

		// Load jail environment files
		char env_path[ENV_PATH_LEN];
		snprintf(env_path, sizeof(env_path),
		    "%s/jails-system/%s/environment", workdir, jname);
		jname_putenv(env_path);

		snprintf(env_path, sizeof(env_path),
		    "%s/jails-system/%s/environment.local", workdir, jname);
		jname_putenv(env_path);

		// Build and exec jexec
		int use_user = (strcmp(user, "root") != 0);
		char *jexec_argv[MAX_JEXEC_ARGS];
		build_jexec_argv(jexec_argv, MAX_JEXEC_ARGS,
		    jname, user, homedir, shell, cmd,
		    use_user, home_set);

		execv("/usr/sbin/jexec", jexec_argv);
		perror("jexec_env: execv failed");
		_exit(JEXEC_EXECV_FAIL_CODE);
	} else if (pid > 0) {
		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			int exit_code = WEXITSTATUS(status);

			// Interactive login (no command): return 0 unless
			// the failure is from execv itself (sentinel).
			if (cmd == NULL && exit_code != JEXEC_EXECV_FAIL_CODE)
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

	return 0; // unreachable
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: jexec_env <jname> [user] [homedir] [shell] [cmd]\n");
		return 1;
	}

	return execute_cmd(argv[1], argv);
}
