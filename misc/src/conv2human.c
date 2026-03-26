// Part of the CBSD Project
// Convert and out bytes to human readable form
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

static int
cbsd_expand_number(const char *s, uint64_t *out)
{
	char *endp = NULL;
	uint64_t v;
	uint64_t mul = 1;
	unsigned long long ull;
	char suf = 0;

	if (!s || !*s || !out)
		return -1;

	errno = 0;
	ull = strtoull(s, &endp, 10);
	if (errno != 0 || endp == s)
		return -1;
	v = (uint64_t)ull;

	if (endp && *endp) {
		suf = (char)tolower((unsigned char)*endp);
		endp++;
		if (*endp != '\0') {
			/* Reject junk like "10mbps". */
			return -1;
		}
		switch (suf) {
		case 'b':
			mul = 1;
			break;
		case 'k':
			mul = 1024ULL;
			break;
		case 'm':
			mul = 1024ULL * 1024ULL;
			break;
		case 'g':
			mul = 1024ULL * 1024ULL * 1024ULL;
			break;
		case 't':
			mul = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
			break;
		case 'p':
			mul = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
			break;
		case 'e':
			mul = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
			break;
		default:
			return -1;
		}
	}

	if (mul != 0 && v > UINT64_MAX / mul)
		return -1;
	*out = v * mul;
	return 0;
}

static void
cbsd_humanize_bytes(char *buf, size_t buflen, uint64_t bytes)
{
	static const char suffixes[] = "BKMGTPE";
	size_t i = 0;
	uint64_t v = bytes;

	if (!buf || buflen == 0)
		return;

	while (v >= 1024 && i < (sizeof(suffixes) - 2)) {
		v /= 1024;
		i++;
	}

	(void)snprintf(buf, buflen, "%" PRIu64 "%c", v, suffixes[i]);
}

#define MAX_VAL_LEN 1024

int
prthumanval(uint64_t bytes)
{
	char buf[6];
	cbsd_humanize_bytes(buf, sizeof(buf), bytes);

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
			for (i = 0; i < (int)strlen(metrics); i++) {
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

		if (cbsd_expand_number(buf, &number) == -1) {
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
