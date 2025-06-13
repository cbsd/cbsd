// Part of the CBSD Project
// Exec cmd via jexec
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

		if (argv[3] != NULL) {
			setenv("HOME",argv[3], 1);
			home_set=1;
		}

		// jexec -d supported in FreeBSD 14.3+
		fp = popen("/usr/local/cbsd/misc/elf_tables --ver /bin/sh", "r");
		if (fp == NULL) {
			fprintf(stderr, "/usr/local/cbsd/misc/elf_tables --ver /bin/sh\n");
			exit(1);
		}

		fgets(buffer, sizeof(buffer), fp);
		pclose(fp);

		freebsd_ver=atoi(buffer);

		if (home_set==1) {
			//reset home_set for FreeBSD < 14.3
			if (freebsd_ver<1403000)
				home_set=0;
		}

		char env_path[512];
		snprintf(env_path, sizeof(env_path), "%s/jails-system/%s/environment", workdir, jname);
		jname_putenv(env_path);
		snprintf(env_path, sizeof(env_path), "%s/jails-system/%s/environment.local", workdir, jname);
		jname_putenv(env_path);

		// Build argv for jexec: {"jexec", jname, "/bin/sh", "-c", argv[2], NULL}
		#define MAX_JEXEC_ARGS 10
		char *jexec_argv[MAX_JEXEC_ARGS];
		jexec_argv[jexec_index++] = "jexec";
		if (strcmp(argv[2],"root")) {
			jexec_argv[jexec_index++] = "-U";
			jexec_argv[jexec_index++] = argv[2]; // user
			// 14.3+
			if (home_set==1) {
				jexec_argv[jexec_index++] = "-d";
				jexec_argv[jexec_index++] = argv[3]; // Homedir
			}
			jexec_argv[jexec_index++] = jname;
			jexec_argv[jexec_index++] = argv[4]; // Shell
			if ( argv[5] != NULL ) {
				jexec_argv[jexec_index++] = "-c";
				jexec_argv[jexec_index++] = argv[5]; // The quoted command string
				jexec_argv[jexec_index++] = NULL;
			} else {
				jexec_argv[jexec_index++] = NULL;
			}
		} else {
			// 14.3+
			if (home_set==1) {
				jexec_argv[jexec_index++] = "-d";
				jexec_argv[jexec_index++] = argv[3]; // Homedir
			}
			jexec_argv[jexec_index++] = jname;
			jexec_argv[jexec_index++] = argv[4]; // Shell
			if ( argv[5] != NULL ) {
				jexec_argv[jexec_index++] = "-c";
				jexec_argv[jexec_index++] = argv[5]; // The quoted command string
				jexec_argv[jexec_index++] = NULL;
			} else {
				jexec_argv[jexec_index++] = NULL;
			}
		}
//		if (argv[2] == NULL) {
//			fprintf(stderr, "No command specified.\n");
//			exit(1);
//		}

		// Execute the command with the new environment
		execv("/usr/sbin/jexec", jexec_argv);
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
