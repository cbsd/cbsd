#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s \"command1 args\" \"command2 args\" ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    int stats_enabled = 0;
    int n = 0;
    char **cmd_argv = malloc((argc - 1) * sizeof(char*));
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-s") == 0) {
            stats_enabled = 1;
        } else {
            cmd_argv[n++] = argv[i];
        }
    }
    if (n < 1) {
        fprintf(stderr, "Usage: %s [-s] \"command1 args\" \"command2 args\" ...\n", argv[0]);
        free(cmd_argv);
        return EXIT_FAILURE;
    }
    pid_t *pids = malloc(n * sizeof(pid_t));
    struct timespec *start_times = stats_enabled ? malloc(n * sizeof(struct timespec)) : NULL;
    struct timespec *end_times = stats_enabled ? malloc(n * sizeof(struct timespec)) : NULL;
    double *elapsed_times = stats_enabled ? malloc(n * sizeof(double)) : NULL;
    double *user_cpus = stats_enabled ? malloc(n * sizeof(double)) : NULL;
    double *sys_cpus = stats_enabled ? malloc(n * sizeof(double)) : NULL;
    long int *rss_values = stats_enabled ? malloc(n * sizeof(long int)) : NULL;
    char ***cmd_args = malloc(n * sizeof(char **));
    int error_found = 0;
    int status;

    for (int i = 0; i < n; i++) {
	// max 64 args per command
        cmd_args[i] = malloc(65 * sizeof(char*));
        int arg_idx = 0;
        char *cmd_copy = strdup(cmd_argv[i]);
        char *token = strtok(cmd_copy, " ");
        while (token && arg_idx < 64) {
            cmd_args[i][arg_idx++] = strdup(token);
            token = strtok(NULL, " ");
        }
        cmd_args[i][arg_idx] = NULL;
        free(cmd_copy);
    }

    for (int i = 0; i < n; i++) {
        if (stats_enabled) clock_gettime(CLOCK_MONOTONIC, &start_times[i]);
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pids[i] == 0) {
            execvp(cmd_args[i][0], cmd_args[i]);
            perror("execvp");
            exit(127);
        }
    }

    for (int finished = 0; finished < n; finished++) {
        struct rusage usage;
        pid_t ended_pid = wait4(-1, &status, 0, stats_enabled ? &usage : NULL);
        if (ended_pid == -1) {
            perror("wait4");
            error_found = 1;
            continue;
        }
        int cmd_idx = -1;
        for (int j = 0; j < n; j++) {
            if (pids[j] == ended_pid) {
                cmd_idx = j;
                break;
            }
        }
        if (cmd_idx == -1) {
            printf("unknown pid: %d\n", ended_pid);
            continue;
        }
        if (stats_enabled) {
            clock_gettime(CLOCK_MONOTONIC, &end_times[cmd_idx]);
            elapsed_times[cmd_idx] = (end_times[cmd_idx].tv_sec - start_times[cmd_idx].tv_sec) +
                                     (end_times[cmd_idx].tv_nsec - start_times[cmd_idx].tv_nsec) / 1e9;
        }
        double user_cpu = 0, sys_cpu = 0;
        long int ru_maxrss = 0;
        if (stats_enabled) {
            user_cpu = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
            sys_cpu = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
            ru_maxrss = usage.ru_maxrss;
            user_cpus[cmd_idx] = user_cpu;
            sys_cpus[cmd_idx] = sys_cpu;
            rss_values[cmd_idx] = ru_maxrss;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                printf("Process %d (%s) error: %d\n", ended_pid, cmd_args[cmd_idx][0], exit_code);
                error_found = 1;
            }
        } else {
            printf("Process %d (%s) error\n", ended_pid, cmd_args[cmd_idx][0]);
            error_found = 1;
        }
        if (stats_enabled) {
            printf("command '%s' success in %.3f sec, CPU: user %.3f c, system %.3f c, RSS: %ld\n",
                   cmd_argv[cmd_idx], elapsed_times[cmd_idx], user_cpu, sys_cpu, ru_maxrss);
        } else {
            printf("command '%s' complete\n", cmd_argv[cmd_idx]);
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; cmd_args[i][j] != NULL; j++) {
            free(cmd_args[i][j]);
        }
        free(cmd_args[i]);
    }
    free(cmd_args);
    free(pids);
    if (stats_enabled) {
        free(start_times);
        free(end_times);
        // sum stats
        double total_elapsed = 0, total_user = 0, total_sys = 0;
        long int total_rss = 0;
        for (int i = 0; i < n; i++) {
            total_elapsed += elapsed_times[i];
            total_user += user_cpus[i];
            total_sys += sys_cpus[i];
            total_rss += rss_values[i];
        }
        printf("\nSum stats: time: %.3f sec, CPU: user %.3f c, system %.3f c, RSS sum: %ld\n",
               total_elapsed, total_user, total_sys, total_rss);
        free(elapsed_times);
        free(user_cpus);
        free(sys_cpus);
        free(rss_values);
    }
    free(cmd_argv);

    if (error_found) {
        printf("some processes terminated with an error.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
