/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1997-2005
 *	Herbert Xu <herbert@gondor.apana.org.au>.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * String functions.
 *
 *	equal(s1, s2)		Return true if strings are equal.
 *	scopy(from, to)		Copy a string.
 *	scopyn(from, to, n)	Like scopy, but checks for overflow.
 *	number(s)		Convert a string of digits to an integer.
 *	is_number(s)		Return true if s is a string of digits.
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <inttypes.h>
#include <stdlib.h>
#include "shell.h"
#include "syntax.h"
#include "error.h"
#include "mystring.h"
#include "memalloc.h"
#include "parser.h"
#include "system.h"


char nullstr[1];		/* zero length string */
const char spcstr[] = " ";
const char snlfmt[] = "%s\n";
const char dolatstr[] = { CTLQUOTEMARK, CTLVAR, VSNORMAL | VSBIT, '@', '=',
			  CTLQUOTEMARK, '\0' };
const char cqchars[] = {
	'\\',
	CTLESC, CTLMBCHAR, CTLQUOTEMARK, 0
};
const char illnum[] = "Illegal number: %s";
const char homestr[] = "HOME";
const char dotdir[] = ".";

//CIX
#include "output.h"
#include "sysexits.h"
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

int optreset;				/* getopt(3) external variable */

enum {
	C_POS,
	C_LEN,
	C_STR,
};

enum {
	C_NUM,
	C_MULTIPLE,
};

enum {
	D_SEARCH,
	D_STR,
};

#define FALSE 0
#define TRUE 1
//CIX

/*
 * equal - #defined in mystring.h
 */

/*
 * scopy - #defined in mystring.h
 */


#if 0
/*
 * scopyn - copy a string from "from" to "to", truncating the string
 *		if necessary.  "To" is always nul terminated, even if
 *		truncation is performed.  "Size" is the size of "to".
 */

void
scopyn(const char *from, char *to, int size)
{

	while (--size > 0) {
		if ((*to++ = *from++) == '\0')
			return;
	}
	*to = '\0';
}
#endif


/*
 * prefix -- see if pfx is a prefix of string.
 */

char *
prefix(const char *string, const char *pfx)
{
	while (*pfx) {
		if (*pfx++ != *string++)
			return 0;
	}
	return (char *) string;
}

void badnum(const char *s)
{
	sh_error(illnum, s);
}

/*
 * Convert a string into an integer of type intmax_t.  Alow trailing spaces.
 */
intmax_t atomax(const char *s, int base)
{
	char *p;
	intmax_t r;

	errno = 0;
	r = strtoimax(s, &p, base);

	/*
	 * Disallow completely blank strings in non-arithmetic (base != 0)
	 * contexts.
	 */
	if (p == s && base)
		badnum(s);

	while (isspace((unsigned char)*p))
	      p++;

	if (*p)
		badnum(s);

	return r;
}

intmax_t atomax10(const char *s)
{
	return atomax(s, 10);
}

/*
 * Convert a string of digits to an integer, printing an error message on
 * failure.
 */

int
number(const char *s)
{
	intmax_t n = atomax10(s);

	if (n < 0 || n > INT_MAX)
		badnum(s);

	return n;
}



/*
 * Check for a valid number.  This should be elsewhere.
 */

//CIX
int
is_number(const char *p)
{
	do {
		if (! is_digit(*p))
			return 0;
	} while (*++p != '\0');
	return 1;
}

/*
 * Produce a possibly single quoted string suitable as input to the shell.
 * The return string is allocated on the stack.
 */

char *
single_quote(const char *s) {
	char *p;

	STARTSTACKSTR(p);

	do {
		char *q;
		size_t len;

		len = strchrnul(s, '\'') - s;

		q = p = makestrspace(len + 3, p);

		*q++ = '\'';
		q = mempcpy(q, s, len);
		*q++ = '\'';
		s += len;

		STADJUST(q - p, p);

		len = strspn(s, "'");
		if (!len)
			break;

		q = p = makestrspace(len + 3, p);

		*q++ = '"';
		q = mempcpy(q, s, len);
		*q++ = '"';
		s += len;

		STADJUST(q - p, p);
	} while (*s);

	USTPUTC(0, p);

	return stackblock();
}

/*
 * Like strdup but works with the ash stack.
 */

char *
sstrdup(const char *p)
{
	size_t len = strlen(p) + 1;
	return memcpy(stalloc(len), p, len);
}

/*
 * Wrapper around strcmp for qsort/bsearch/...
 */
int
pstrcmp(const void *a, const void *b)
{
	return strcmp(*(const char *const *) a, *(const char *const *) b);
}

/*
 * Find a string is in a sorted array.
 */
const char *const *
findstring(const char *s, const char *const *array, size_t nmemb)
{
	return bsearch(&s, array, nmemb, sizeof(const char *), pstrcmp);
}

//CIX
//int
//is_number(const char *p)
//{
//	const char *q;

//	if (*p == '\0')
//		return 0;
//	while (*p == '0')
//		p++;
//	for (q = p; *q != '\0'; q++)
//		if (!is_digit(*q))
//			return 0;
//	if (q - p > 10 || (q - p == 10 && memcmp(p, "2147483647", 10) > 0))
//		return 0;
//	return 1;
//}

int
is_numbercmd(int argc, char **argv)
{
	if (argv[1])
		return is_number(argv[1]);
	else
		return 1;
}

int
strlencmd(int argc, char **argv)
{
	if (argv[1])
		out1fmt("%u", (unsigned int)strlen(argv[1]));
	else
		out1fmt("0");
	return 0;
}

int
substr_usage(void)
{
	out1fmt("Substring\n");
	out1fmt("require: --pos, --len, --str (--len=0 - read until end)\n");
	return (EX_USAGE);
}

int
strpos_usage(void)
{
	out1fmt(
	    "Find first include of --search in --str. Return 0 if no any match\n");
	out1fmt("require: --search, --str\n");
	return (EX_USAGE);
}

int
substrcmd(int argc, char **argv)
{
	char *pointer;
	int optcode = 0;
	int option_index = 0;
	char *str = NULL;
	int pos = 0;
	int len = 0;

	struct option long_options[] = { { "pos", required_argument, 0, C_POS },
		{ "len", required_argument, 0, C_LEN },
		{ "str", required_argument, 0, C_STR },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc != 4)
		substr_usage();

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_POS:
			pos = atoi(optarg);
			break;
		case C_LEN:
			len = atoi(optarg);
			break;
		case C_STR:
			str = malloc(strlen(optarg) + 1);
			memset(str, 0, strlen(optarg) + 1);
			strcpy(str, optarg);
			break;
		}
	} // while

	if (len == 0)
		len = strlen(str);

	// zero for getopt* variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (str == NULL)
		return 1;
	/* Clamp pos/len to string bounds to avoid overruns */
	size_t start = 0;
	size_t slen = strlen(str);
	if (pos < 1)
		pos = 1;
	start = (size_t)(pos - 1);
	if (start > slen)
		start = slen;
	if (len < 0)
		len = 0;
	if ((size_t)len > (slen - start))
		len = (int)(slen - start);

	pointer = malloc((size_t)len + 1);

	if (pointer == NULL) {
		out1fmt("Unable to allocate memory.\n");
		free(str);
		return 1;
	}

	memcpy(pointer, str + start, (size_t)len);
	pointer[len] = '\0';
	out1fmt("%s", pointer);
	free(pointer);
	free(str);
	return 0;
}

int
strposcmd(int argc, char **argv)
{
	int optcode = 0;
	int option_index = 0;
	char *str = NULL;
	char *search = NULL;
	int pos = 0;

	struct option long_options[] = { { "search", required_argument, 0,
					     D_SEARCH },
		{ "str", required_argument, 0, D_STR },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc != 3)
		strpos_usage();

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case D_SEARCH:
			search = malloc(strlen(optarg) + 1);
			memset(search, 0, strlen(optarg) + 1);
			strcpy(search, optarg);
			break;
		case D_STR:
			str = malloc(strlen(optarg) + 1);
			memset(str, 0, strlen(optarg) + 1);
			strcpy(str, optarg);
			break;
		}
	} // while

	// zero for getopt* variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (str == NULL)
		return 1;

	char *p = strstr(str, search);
	if (p)
		pos = p - str;

	if (pos < 0)
		pos = 0;
	return pos;
}

int
roundup_usage(void)
{
	out1fmt("roundup\n");
	out1fmt("require: --num, --multiple\n");
	out1fmt("  sample: roundup --num=1477 --multiple=500\n");
	return (EX_USAGE);
}

// roundup num by multiple
// todo: long long? :
//   roundup --num=1231332132132132132 --multiple=100
//   roundup --num=12313321321321321321 --multiple=100
int
roundupcmd(int argc, char **argv)
{
	unsigned long long numtoround = 0;
	unsigned long long multiple = 0;
	int optcode = 0;
	int option_index = 0;

	struct option long_options[] = { { "num", required_argument, 0, C_NUM },
		{ "multiple", required_argument, 0, C_MULTIPLE },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	// zero for getopt* variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (argc != 3)
		roundup_usage();

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_NUM:
			numtoround = atoll(optarg);
			break;
		case C_MULTIPLE:
			multiple = atol(optarg);
			break;
		}
	} // while

	if (multiple == 0) {
		out1fmt("%llu", numtoround);
		return 0;
	}

	unsigned long long rounddown = ((unsigned long long)(numtoround) /
					   multiple) *
	    multiple;
	unsigned long long roundup = rounddown + multiple;
	unsigned long long roundcalc = roundup;

	out1fmt("%llu", roundcalc);
	return 0;
}
//CIX
