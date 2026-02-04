// Part of the CBSD Project
// Similar to jexec_env but execute command in hoster, but call /usr/sbin/jail instead of jexec
// In the long term, this could be used to run unprivileged containers (as a user other than root)
// ${miscdir}/daemonize -e /usr/jails/ftmp/jstart.test.err -p /usr/jails/ftmp/jstart.test.88952 /usr/bin/nice -n 1 /usr/sbin/jail -f /usr/jails/ftmp/test.conf -c test
// -> ${NICE_CMD} -n ${nice} ${SETFIB} ${CPUSET} ${miscdir}/exec_envjail /usr/jails/ftmp/test.conf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 256

void jname_putenv(const char *path)
{
	// Read environment variables from the specified file
	FILE *file = fopen(path, "r");
	if (file) {
		char line[MAX_LINE];
		while (fgets(line, sizeof(line), file)) {
			// Remove newline character
			line[strcspn(line, "\n")] = 0;
			// Skip empty lines or comments
			if (line[0] == '\0' || line[0] == '#') continue;
			// Split at the first '='
			char *eq = strchr(line, '=');
			if (!eq) continue; // Invalid line
			*eq = '\0';
			char *name = line;
			char *value = eq + 1;
			setenv(name, value, 1); // 1 to overwrite existing
		}
		fclose(file);
	}
//	else {
//		perror("Failed to open environment file");
//	}
}

int execute_cmd(char *jname, char **argv)
{
	char *workdir = getenv("workdir");
	const char *term;
	const char *blocksize;
	int home_set=0, jexec_index=0, freebsd_ver=0;
	FILE *fp;
	char buffer[128];

	if (!workdir) {
		fprintf(stderr, "Environment variable 'workdir' is not set.\n");
		exit(1);
	}
	if (!jname) {
		fprintf(stderr, "Jail name is required.\n");
		exit(1);
	}

	// inherit TERM/BLOCKSIZE by default
	term = getenv("TERM");
	blocksize = getenv("BLOCKSIZE");

	pid_t pid = fork();

	if (pid == 0) {
		// Child process: clear environment and load from jail env files
		char *cleanenv[1];
		extern char **environ;
		environ = cleanenv;
		cleanenv[0] = NULL;

		// inherit TERM by default
		if (term != NULL)
			setenv("TERM", term, 1);

		if (blocksize != NULL)
			setenv("BLOCKSIZE", blocksize, 1);

		char env_path[512];
		snprintf(env_path, sizeof(env_path), "%s/jails-system/%s/environment", workdir, jname);
		jname_putenv(env_path);
		snprintf(env_path, sizeof(env_path), "%s/jails-system/%s/environment.local", workdir, jname);
		jname_putenv(env_path);

		// Build argv for jexec: {"jexec", jname, "/bin/sh", "-c", argv[2], NULL}
		#define MAX_JEXEC_ARGS 10
		char *jexec_argv[MAX_JEXEC_ARGS];
		jexec_argv[jexec_index++] = "jail";

		jexec_argv[jexec_index++] = "-f";
		jexec_argv[jexec_index++] = argv[2]; // config
		jexec_argv[jexec_index++] = "-c";
		jexec_argv[jexec_index++] = jname;
		jexec_argv[jexec_index++] = NULL;

		// Execute the command with the new environment
		execv("/usr/sbin/jail", jexec_argv);
		// If execv returns, it failed
		perror("execv failed");
		exit(1);
	} else if (pid > 0) {
		wait(NULL);
	} else {
		perror("fork failed");
		exit(1);
	}

	return 0;
}

int main(int argc, char **argv)
{
	char *jname = NULL;

	jname=argv[1];

	execute_cmd(jname, argv);
	return 0;
}
