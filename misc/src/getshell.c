#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <passwd_file> <username>\n", argv[0]);
		return 1;
	}

	const char *passwd_path = argv[1];
	const char *username = argv[2];
	FILE *fp = fopen(passwd_path, "r");
	if (!fp) {
		printf("/bin/sh\n");
		return 1;
	}

	char line[MAX_LINE];
	int found = 0;
	while (fgets(line, sizeof(line), fp)) {
		// Remove newline
		line[strcspn(line, "\n")] = 0;

		// Format: user:passwd:uid:gid:gecos:home:shell
		char *saveptr;
		char *user = strtok_r(line, ":", &saveptr);
		if (!user) continue;
		if (strcmp(user, username) == 0) {
			// Skip to shell field
			char *field = NULL;
			for (int i = 0; i < 5; ++i) field = strtok_r(NULL, ":", &saveptr);
			char *shell = strtok_r(NULL, ":", &saveptr);
			printf("%s\n", shell ? shell : "/bin/sh");
		found = 1;
		break;
	}
	}
	fclose(fp);

	if (!found) {
		printf("/bin/sh\n");
		return 1;
	}
	return 0;
}
