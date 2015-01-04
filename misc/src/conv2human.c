// Part of the CBSD Project
// Convert and out bytes to human readable form

// todo:learn for float to expand_number.E.g:1.1 g as 1100 m

#include <stdio.h>
#include <libutil.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef __DragonFly__
#include "expand_number.c"
#endif

int
prthumanval(uint64_t bytes)
{
	char	buf[6];
	int	flags;
	//flags = HN_NOSPACE | HN_DECIMAL | HN_DIVISOR_1000;
	//flags = HN_NOSPACE | HN_DECIMAL;
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
	int		i = 0;
	uint64_t	number;

	if (argc != 2)
		return (1);

	if (is_number(argv[1]) == 1) {
		//num num
		if (expand_number(argv[1], &number) == -1) {
			//invalid value for val argument
			// printf("Bad value\n");
			exit(1);
		} else {
			printf("%lu", (unsigned long)number);
			exit(0);
		}
	} else
		prthumanval(atol(argv[1]));
	return 0;
}
