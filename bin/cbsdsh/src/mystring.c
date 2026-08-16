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
#include "input.h"
#include "options.h"
#include "system.h"
#include "var.h"


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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

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
		return is_number(argv[1]) ? 0 : 1;
	else
		return 1;
}

/*
 * gettime VAR
 *   Assign current epoch seconds (like `date +%s`) to VAR.
 *   Avoids external commands and command substitution.
 */
int
gettimecmd(int argc, char **argv)
{
	struct timeval tv;
	char buf[32];
	const char *varname;

	if (argc != 2)
		return 1;

	varname = argv[1];
	if (!varname || !*varname || !goodname(varname))
		return 1;

	if (gettimeofday(&tv, NULL) != 0) {
		setvar(varname, "", 0);
		return 1;
	}

	snprintf(buf, sizeof(buf), "%ld", (long)tv.tv_sec);
	setvar(varname, buf, 0);
	return 0;
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
	int have_pos = 0;
	int have_len = 0;
	int have_str = 0;

	struct option long_options[] = { { "pos", required_argument, 0, C_POS },
		{ "len", required_argument, 0, C_LEN },
		{ "str", required_argument, 0, C_STR },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc != 4)
		return substr_usage();

	/*
	 * Reset getopt state for repeated invocations.
	 * capture() executes builtins in-process (no fork), so getopt globals
	 * can leak state between calls if we don't reset before parsing.
	 */
	optind = 1;
	optopt = 0;
	opterr = 0;
	optreset = 1;

	while (TRUE) {
		/* NetBSD does not provide getopt_long_only() in base libc. */
		optcode = getopt_long(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_POS:
			pos = atoi(optarg);
			have_pos = 1;
			break;
		case C_LEN:
			len = atoi(optarg);
			have_len = 1;
			break;
		case C_STR:
		{
			char *tmp = malloc(strlen(optarg) + 1);

			if (tmp == NULL) {
				out1fmt("Unable to allocate memory.\n");
				return 1;
			}
			/* Copy trailing '\0' too */
			strcpy(tmp, optarg);
			free(str);
			str = tmp;
			have_str = 1;
			break;
		}
		}
	} // while

	if (str == NULL)
		return 1;
	if (!have_pos || !have_len || !have_str)
		return substr_usage();

	/* --len=0 means "read until end" (see substr_usage). */
	if (len == 0)
		len = strlen(str);

	// zero for getopt* variables for next execute
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

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
	int have_search = 0;
	int have_str = 0;
	int rc = 1;

	struct option long_options[] = { { "search", required_argument, 0,
					     D_SEARCH },
		{ "str", required_argument, 0, D_STR },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc != 3)
		return strpos_usage();

	/* Reset getopt state for repeated invocations. */
	optind = 1;
	optopt = 0;
	opterr = 0;
	optreset = 1;

	while (TRUE) {
		/* NetBSD does not provide getopt_long_only() in base libc. */
		optcode = getopt_long(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case D_SEARCH:
		{
			char *tmp = malloc(strlen(optarg) + 1);

			if (tmp == NULL)
				goto out;
			strcpy(tmp, optarg);
			free(search);
			search = tmp;
			have_search = 1;
			break;
		}
		case D_STR:
		{
			char *tmp = malloc(strlen(optarg) + 1);

			if (tmp == NULL)
				goto out;
			strcpy(tmp, optarg);
			free(str);
			str = tmp;
			have_str = 1;
			break;
		}
		}
	} // while

	if (!have_search || !have_str)
		rc = strpos_usage();
	else if (search == NULL || str == NULL)
		rc = 1;
	else {
		char *p = strstr(str, search);
		if (p)
			pos = (int)(p - str);
		rc = pos;
	}

	/* zero for getopt* variables for next execute (best-effort) */
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

out:
	free(search);
	free(str);
	return rc;
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
	int have_num = 0;
	int have_multiple = 0;
	int optcode = 0;
	int option_index = 0;

	struct option long_options[] = { { "num", required_argument, 0, C_NUM },
		{ "multiple", required_argument, 0, C_MULTIPLE },
		/* End of options marker */
		{ 0, 0, 0, 0 } };

	if (argc != 3)
		return roundup_usage();

	/* Reset getopt state for repeated invocations. */
	optind = 1;
	optopt = 0;
	opterr = 0;
	optreset = 1;

	while (TRUE) {
		/* NetBSD does not provide getopt_long_only() in base libc. */
		optcode = getopt_long(argc, argv, "", long_options,
		    &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_NUM:
		{
			char *endp = NULL;

			errno = 0;
			numtoround = strtoull(optarg, &endp, 10);
			if (errno != 0 || endp == optarg || (endp && *endp != '\0'))
				return roundup_usage();
			have_num = 1;
			break;
		}
		case C_MULTIPLE:
		{
			char *endp = NULL;

			errno = 0;
			multiple = strtoull(optarg, &endp, 10);
			if (errno != 0 || endp == optarg || (endp && *endp != '\0'))
				return roundup_usage();
			have_multiple = 1;
			break;
		}
		}
	} // while

	// zero for getopt* variables for next execute (best-effort)
	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (!have_num || !have_multiple)
		return roundup_usage();

	if (multiple == 0) {
		out1fmt("%llu", numtoround);
		return 0;
	}

	/* If already multiple, keep it; otherwise round up to next multiple. */
	unsigned long long rem = numtoround % multiple;
	unsigned long long roundcalc = (rem == 0) ? numtoround : (numtoround - rem + multiple);

	out1fmt("%llu", roundcalc);
	return 0;
}

enum {
	S_STR,
	S_CHARS,
};

int
strstrip_usage(void)
{
	out1fmt("Strip characters from string\n");
	out1fmt("require: --str, --chars\n");
	out1fmt("  sample: strstrip --str='\"hello\"' --chars='\"'\n");
	return (EX_USAGE);
}

int
strstripcmd(int argc, char **argv)
{
	int optcode = 0;
	int option_index = 0;
	char *str = NULL;
	char *chars = NULL;
	int have_str = 0;
	int have_chars = 0;
	char bitmap[256];

	struct option long_options[] = {
		{ "str", required_argument, 0, S_STR },
		{ "chars", required_argument, 0, S_CHARS },
		{ 0, 0, 0, 0 }
	};

	if (argc != 3)
		return strstrip_usage();

	optind = 1;
	optopt = 0;
	opterr = 0;
	optreset = 1;

	while (TRUE) {
		optcode = getopt_long(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case S_STR:
			str = optarg;
			have_str = 1;
			break;
		case S_CHARS:
			chars = optarg;
			have_chars = 1;
			break;
		}
	}

	optarg = NULL;
	optind = 0;
	optopt = 0;
	opterr = 0;
	optreset = 0;

	if (!have_str || !have_chars)
		return strstrip_usage();

	memset(bitmap, 0, sizeof(bitmap));
	for (const char *p = chars; *p; p++)
		bitmap[(unsigned char)*p] = 1;

	for (const char *p = str; *p; p++) {
		if (!bitmap[(unsigned char)*p])
			outc(*p, &output);
	}
	flushout(&output);

	return 0;
}


typedef struct {
    const char *arg;   // source argv[i]
    char *key;         // strings key
    const char *val;   // pointer to argv[i] after '='
    char *val_alloc;   // allocated value (if we had to join/unquote)
    int append;        // 1 when +=
    int applied;       // 1 when applied
} kv_arg_t;

static const char *skip_ws(const char *s)
{
	while (*s && isspace((unsigned char)*s))
		s++;
	return s;
}

static void rstrip_ws(char *s)
{
	size_t n = strlen(s);
	while (n > 0 && isspace((unsigned char)s[n - 1])) {
		s[n - 1] = '\0';
		n--;
	}
}

static void extract_existing_value_after_eq(const char *after_eq, char *out, size_t maxlen)
{
	const char *s = skip_ws(after_eq);
	size_t len = 0;

	out[0] = '\0';
	if (maxlen == 0)
		return;

	if (*s == '"' || *s == '\'') {
		char q = *s++;
		const char *e = strchr(s, q);
		if (!e)
			e = s + strlen(s);
		len = (size_t)(e - s);
	} else {
		const char *e = s;
		while (*e && *e != '\n' && *e != '\r')
			e++;
		len = (size_t)(e - s);
	}

	if (len >= maxlen)
		len = maxlen - 1;
	memcpy(out, s, len);
	out[len] = '\0';
	rstrip_ws(out);
}

static void escape_for_double_quotes(const char *in, char *out, size_t outsz)
{
	size_t j = 0;
	for (size_t i = 0; in[i] && j + 1 < outsz; i++) {
		unsigned char c = (unsigned char)in[i];
		if ((c == '\\' || c == '"') && j + 2 < outsz) {
			out[j++] = '\\';
			out[j++] = (char)c;
		} else {
			out[j++] = (char)c;
		}
	}
	out[j] = '\0';
}

static int parse_kv_args(int argc, char *argv[], kv_arg_t **out_args, int *out_count) {
    kv_arg_t *args = calloc((size_t)argc, sizeof(*args));
    if (!args) return -1;

    int n = 0;
    for (int i = 0; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!eq) continue;

        int append = (eq > argv[i] && *(eq - 1) == '+');
        size_t key_len = append ? (size_t)(eq - argv[i] - 1) : (size_t)(eq - argv[i]);
        if (key_len == 0) continue;

        args[n].arg = argv[i];
        args[n].append = append;
        args[n].val = eq + 1;
        args[n].key = strndup(argv[i], key_len);
        if (!args[n].key) {
            for (int j = 0; j < n; j++) {
                free(args[j].key);
                free(args[j].val_alloc);
            }
            free(args);
            return -1;
        }

        /*
         * If caller passed an unquoted expansion like:
         *   pkglist="bash mc"
         * it can arrive split into argv as:
         *   pkglist="bash
         *   mc"
         * Here we detect a value starting with a quote (or escaped quote)
         * but missing its closing quote in the same argv element, and join
         * subsequent argv elements until the closing quote is found.
         */
        const char *v = args[n].val;
        char q = 0;
        int v_escaped = 0;
        if (v[0] == '"' || v[0] == '\'') {
            q = v[0];
        } else if (v[0] == '\\' && (v[1] == '"' || v[1] == '\'')) {
            q = v[1];
            v_escaped = 1;
        }
        if (q) {
            /* Try to locate closing quote in this and following argv tokens */
            size_t cap = 0;
            size_t len = 0;
            char *buf = NULL;

            /* helper: append one character */
            #define APPCH(ch) do { \
                if (len + 1 >= cap) { \
                    cap = cap ? cap * 2 : 64; \
                    char *nbuf = realloc(buf, cap); \
                    if (!nbuf) { free(buf); goto nojoin; } \
                    buf = nbuf; \
                } \
                buf[len++] = (ch); \
            } while (0)

            /* helper: append string */
            #define APPSTR(s) do { \
                const char *_s = (s); \
                while (*_s) APPCH(*_s++); \
            } while (0)

            const char *p = v + (v_escaped ? 2 : 1); /* skip opening quote */
            int closed = 0;

            for (;;) {
                for (; *p; p++) {
                    if (*p == '\\' && p[1]) {
                        /* keep escaped char as-is */
                        APPCH(p[1]);
                        p++;
                        continue;
                    }
                    if (*p == q) {
                        closed = 1;
                        p++;
                        break;
                    }
                    APPCH(*p);
                }
                if (closed)
                    break;

                /* need next argv token */
                if (i + 1 >= argc)
                    break;
                i++;
                APPCH(' ');
                p = argv[i];
            }

            APPCH('\0');

            args[n].val_alloc = buf;
            args[n].val = args[n].val_alloc;

            #undef APPCH
            #undef APPSTR
        }
nojoin:
        n++;
    }

    *out_args = args;
    *out_count = n;
    return 0;
}

static void free_kv_args(kv_arg_t *args, int n) {
    for (int i = 0; i < n; i++) {
        free(args[i].key);
        free(args[i].val_alloc);
    }
    free(args);
}

static long long monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int try_lock_with_timeout(int fd, int timeout_ms) {
    long long start = monotonic_ms();
    for (;;) {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) return 0;
        if (errno != EWOULDBLOCK && errno != EAGAIN) return -1;

        long long now = monotonic_ms();
        if (timeout_ms >= 0 && (now - start) >= timeout_ms) return 1; // timeout

        struct timespec req = { .tv_sec = 0, .tv_nsec = 100 * 1000 * 1000 }; // 100ms
        (void)nanosleep(&req, NULL);
    }
}

/*
 * Validate shell syntax using the interpreter's own parser.
 * Equivalent to running this dash with '-n' on the file, but without
 * spawning an external /bin/sh.
 *
 * Returns 0 if syntax is OK, non-zero otherwise.
 */
static int dash_syntax_check_file(const char *path)
{
	struct jmploc jmploc;
	struct jmploc *volatile savehandler = handler;
	char save_nflag = nflag;
	int pushed = 0;
	int err = 0;

	if (setjmp(jmploc.loc)) {
		err = 1;
		goto out;
	}

	handler = &jmploc;
	nflag = 1;

	if (setinputfile(path, INPUT_PUSH_FILE) < 0) {
		err = 1;
		goto out;
	}
	pushed = 1;

	{
		struct stackmark smark;
		union node *n;

		setstackmark(&smark);
		for (; (n = parsecmd(0)) != NEOF; popstackmark(&smark)) {
			/* parse only; eval is skipped when nflag is set */
			(void)n;
		}
		popstackmark(&smark);
	}

out:
	if (pushed)
		popfile();
	nflag = save_nflag;
	handler = savehandler;
	return err;
}


int updateconf(int argc, char *argv[]) {
    if (argc < 3 || !argv[1] || !*argv[1]) {
        fprintf(stderr, "usage: updateconf <path_to_file> key=val [key2=val2 ...]\n");
        return EX_USAGE;
    }

    const char *filename = argv[1];
    char temp_path[PATH_MAX];
    snprintf(temp_path, sizeof(temp_path), "%s.tmpXXXXXX", filename);

    char lock_path[PATH_MAX];
    snprintf(lock_path, sizeof(lock_path), "%s.lock", filename);
    int lock_fd = open(lock_path, O_CREAT | O_RDWR, 0600);
    if (lock_fd < 0) return -1;
    int lock_rc = try_lock_with_timeout(lock_fd, 2000);
    if (lock_rc < 0) { close(lock_fd); return -1; }
    if (lock_rc == 1) { close(lock_fd); lock_fd = -1; } // timeout -> force

    struct stat st;
    int have_st = (stat(filename, &st) == 0);

    int fd = mkstemp(temp_path);
    if (fd == -1) {
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -1;
    }

    if (have_st) {
        (void)fchmod(fd, st.st_mode);
        (void)fchown(fd, st.st_uid, st.st_gid);
    }

    FILE *tmp_fp = fdopen(fd, "w");
    if (!tmp_fp) {
        close(fd);
        unlink(temp_path);
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -1;
    }

    kv_arg_t *args = NULL;
    int n_args = 0;
    if (parse_kv_args(argc - 2, argv + 2, &args, &n_args) != 0) {
        fclose(tmp_fp);
        unlink(temp_path);
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -1;
    }

    FILE *src_fp = fopen(filename, "r");
    if (src_fp) {
        char line[2048];
        while (fgets(line, sizeof(line), src_fp)) {
            int replaced = 0;
            for (int i = 0; i < n_args; i++) {
                size_t key_len = strlen(args[i].key);
                const char *p = line;
                p = skip_ws(p);
                if (strncmp(p, args[i].key, key_len) == 0) {
                    p += key_len;
                    p = skip_ws(p);
                    if (*p != '=')
                        continue;
                    p++; /* after '=' */
                    if (args[i].append) {
                        char old_val[1024];
                        char esc_old[2048];
                        char esc_new[2048];
                        extract_existing_value_after_eq(p, old_val, sizeof(old_val));
                        escape_for_double_quotes(old_val, esc_old, sizeof(esc_old));
                        escape_for_double_quotes(args[i].val, esc_new, sizeof(esc_new));
                        if (esc_old[0] != '\0')
                            fprintf(tmp_fp, "%s=\"%s %s\"\n", args[i].key, esc_old, esc_new);
                        else
                            fprintf(tmp_fp, "%s=\"%s\"\n", args[i].key, esc_new);
                    } else {
                        char esc_new[2048];
                        escape_for_double_quotes(args[i].val, esc_new, sizeof(esc_new));
                        fprintf(tmp_fp, "%s=\"%s\"\n", args[i].key, esc_new);
                    }
                    args[i].applied = 1;
                    replaced = 1;
                    break;
                }
            }
            if (!replaced) fputs(line, tmp_fp);
        }
        fclose(src_fp);
    }

    // add new
    for (int i = 0; i < n_args; i++) {
        if (!args[i].applied) {
            char esc_new[2048];
            escape_for_double_quotes(args[i].val, esc_new, sizeof(esc_new));
            fprintf(tmp_fp, "%s=\"%s\"\n", args[i].key, esc_new);
        }
    }

    fclose(tmp_fp);
    free_kv_args(args, n_args);

    int sh_rc = dash_syntax_check_file(temp_path);
    if (sh_rc != 0) {
        fprintf(stderr, "updateconf error: wrong syntax.\n");
        unlink(temp_path);
        if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
        return -2;
    }

    int rc = rename(temp_path, filename);
    if (lock_fd >= 0) { (void)flock(lock_fd, LOCK_UN); close(lock_fd); }
    return rc;
}
