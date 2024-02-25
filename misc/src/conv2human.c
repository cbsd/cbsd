// Part of the CBSD Project
// Convert and out bytes to human readable form
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef __Linux__
#include <bsd/libutil.h>
#else
#include <libutil.h>
#endif
#include <string.h>

#ifdef __DragonFly__
#include "expand_number.c"
#endif

#define MAX_VAL_LEN 1024

int
prthumanval(uint64_t bytes)
{
	char buf[6];
	int flags;
	// flags = HN_NOSPACE | HN_DECIMAL | HN_DIVISOR_1000;
	// flags = HN_NOSPACE | HN_DECIMAL;
	flags = HN_NOSPACE;

	humanize_number(buf, sizeof(buf) - 1, bytes, "", HN_AUTOSCALE, flags);

	(void)printf("%s", buf);
	return 0;
}

int
is_number(const char *p)
{
	do {
		if (!isdigit(*p) && (*p) != '.') {
			return 1;
		}
	} while (*++p != '\0');
	return 0;
}

int
main(int argc, char *argv[])
{
	int i = 0;
	uint64_t number;
	int is_float = 0;
	char metrics[] = "bkmgtpe";
	char in_metrics;
	int len = 0;
	int in_index = -1;
	int new_val;
	char stringnum[MAX_VAL_LEN];
	char buf[MAX_VAL_LEN];
	float f = 0;

	if (argc != 2) {
		return (1);
	}

	len = strlen(argv[1]);

	if (len > MAX_VAL_LEN) {
		fprintf(stderr, "too long: %s\n", argv[1]);
		exit(1);
	}

	memset(stringnum, 0, sizeof(stringnum));
	memset(buf, 0, sizeof(buf));

	if (is_number(argv[1]) == 1) {
		// is float?
		for (i = 0; i < len; i++) {
			if (argv[1][i] == '.') {
				is_float = 1;
			}
			if ((argv[1][i] >= '.') &&
			    (argv[1][i] <= '9')) { // '.' - 46 code
				stringnum[i] = argv[1][i];
			}
		}
		if (is_float == 1) {
			char in_metrics = argv[1][len - 1];
			for (i = 0; i < strlen(metrics); i++) {
				if (metrics[i] == in_metrics) {
					in_index = i;
					break;
				}
			}
			if (in_index < 0) {
				fprintf(stderr,
				    "unable to determine index of metric: %s\n",
				    argv[1]);
				exit(1);
			}
			f = atof(stringnum);
			new_val = f * 1024; // convert to prev metrics
			sprintf(buf, "%d%c", new_val,
			    metrics[in_index - 1]); // and shift metrics index
		} else {
			strncpy(buf, argv[1], strlen(argv[1]));
		}

		if (expand_number(buf, &number) == -1) {
			// invalid value for val argument
			//  printf("Bad value\n");
			exit(1);
		} else {
			printf("%lu", (unsigned long)number);
			exit(0);
		}
	} else {
		prthumanval(atol(argv[1]));
	}
	return 0;
}
