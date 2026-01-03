#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "spawn_task.h"
#include "output.h"
#include "var.h"

int logging_type = 0;
char log_file_name[1024];

int set_output(int outfd[2]);

int
set_output(int outfd[2])
{
	// set up output
	switch (logging_type) {
	case log_type_mail:
	case log_type_dev_null:
		outfd[1] = open("/dev/null", O_WRONLY);
		break;

	case log_type_file_trunc:
		outfd[1] = open(log_file_name, O_CREAT | O_WRONLY | O_TRUNC,
		    0644);
		break;

	case log_type_file_append:
		outfd[1] = open(log_file_name, O_CREAT | O_WRONLY | O_APPEND,
		    0644);
		break;

	case log_type_tty:
		outfd[1] = open("/dev/tty", O_WRONLY | O_NOCTTY);
		break;

		if (outfd[1] < 0) {
			//		syslog(LOG_ERR, "Couldn't set up job
			// output file descriptors: %m");
			_exit(1);
		}
	}

	return 0;
}

int
spawncmd(int argc, char **argv)
{
	pid_t pid, logger_pid = -1;
	int child_status = 0, infd = -1, outfd[2] = { -1, -1 };
	//	char ** env = NULL;
	char *workdir = lookupvar("workdir");
	char env_workdir[10 + strlen(workdir)];

	sprintf(env_workdir, "workdir=%s", workdir);

	char *env[] = {
		"PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin",
		"NOCOLOR=1", "INTER=0", env_workdir, NULL
	};
	//	char *shell="/bin/sh";
	char *shell = "/usr/local/bin/cbsd";
	int res = 0;
	int i = 0;
	char *command = NULL;
	char *tmp;
	int jobid = 0;

	if (argc < 4) {
		out1fmt("spawntask: <jobid> <logfile> <cmd>\n");
		return 0;
	}

	for (i = 3; i < argc; i++)
		res += strlen(argv[i]) + 1;

	if (res) {
		command = (char *)malloc(res);
		tmp = command;
		for (i = 3; i < argc; i++) {
			strcpy(tmp, argv[i]);
			tmp += strlen(tmp);
			*tmp = ' ';
			tmp++;
		}
		tmp[-1] = 0;
	}

	logging_type = log_type_file_trunc;
	strcpy(log_file_name, argv[2]);
	jobid = atoi(argv[1]);
	out1fmt("spawn [job: %d, logfile: %s]: %s\n", jobid,log_file_name, command);

	if ((infd = open("/dev/null", O_RDONLY)) < 0) {
		// syslog(LOG_ERR, "Couldn't open file %s: %m", "/dev/null");
		return 1;
	}

	set_output(outfd);

	if ((pid = fork()) == 0) {
		// syslog(LOG_DEBUG, "exec'ing %s", command);
		dup2(infd, STDIN_FILENO);
		dup2(outfd[1], STDOUT_FILENO);
		dup2(outfd[1], STDERR_FILENO);

		/* close any open files */
		close(infd);
		close(outfd[0]);
		close(outfd[1]);

		// closelog();

		setsid();

		execle(shell, shell, "-c", command, NULL, env);

		// doLogOpen(LOG_CRON, LOG_PID, "%s(child)", progName);
		// syslog(LOG_ERR, "Couldn't exec '%s -c %s': %m", shell,
		// command);

		_exit(1);
	} else if (pid == -1) {
		/* error */
		// syslog(LOG_ERR, "Couldn't fork off child: %m");
		return 1;
	}

	close(infd);
	close(outfd[1]);
	close(outfd[0]);

	/* we make sure the command process finished successfully here */
	waitpid(pid, &child_status, 0);

	/* here we deal with the logger child process */
	if (logger_pid != -1)
		waitpid(logger_pid, &child_status, 0);

	return child_status;
}
