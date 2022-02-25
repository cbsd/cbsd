#include <sys/param.h>
#include <sys/jail.h>

#ifdef __DragonFly__

#include <sys/types.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "output.h"

int
cbsdjlscmd(int argc, char **argv)
{
	size_t len;
	char *jls; /* Jail list */
	char *curpos;
	char *nextpos;

	if (sysctlbyname("jail.list", NULL, &len, NULL, 0) == -1)
		err(1, "sysctlbyname(): jail.list");
retry:
	if (len == 0)
		return (0);

	jls = malloc(len);
	if (jls == NULL)
		err(1, "malloc failed");

	if (sysctlbyname("jail.list", jls, &len, NULL, 0) == -1) {
		if (errno == ENOMEM) {
			free(jls);
			goto retry;
		}
		err(1, "sysctlbyname(): jail.list");
	}
	//      printf("JID\tHostname\tPath\t\tIPs\n");
	curpos = jls;
	while (curpos) {
		char *str_jid;
		char *str_host;
		char *str_path;
		char *str_ips;
		char *jname;
		nextpos = strchr(curpos, '\n');
		if (nextpos)
			*nextpos++ = 0;
		str_jid = strtok(curpos, " ");
		str_host = strtok(NULL, " ");
		str_path = strtok(NULL, " ");
		str_ips = strtok(NULL, "\n");

		jname = strrchr(str_path, '/') + 1;

		// use vars to elimiate clang/gcc warning
		// (-Wunused-but-set-variable)
		free(str_host);
		free(str_ips);

		out1fmt("%s %s\n", str_jid, jname);
		curpos = nextpos;
	}
	free(jls);
	return 0;
}
// no jail.h
#else
#include <jail.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "output.h"

#define JP_USER 0x01000000
#define JP_OPT 0x02000000

#define PRINT_DEFAULT 0x01
#define PRINT_HEADER 0x02
#define PRINT_NAMEVAL 0x04
#define PRINT_QUOTED 0x08
#define PRINT_SKIP 0x10
#define PRINT_VERBOSE 0x20
#define PRINT_JAIL_NAME 0x40

static struct jailparam *params;
static int *param_parent;
static int nparams;

static int add_param(const char *name, void *value, size_t valuelen,
    struct jailparam *source, unsigned flags);
static int sort_param(const void *a, const void *b);
static int print_jail(int pflags, int jflags);
static int print_jids(int pflags, int jflags);

int
cbsdjlscmd(int argc, char **argv)
{
	int c, jflags, jid, lastjid, pflags, jid_only = 0;

	pflags = jflags = jid = 0;
	while ((c = getopt(argc, argv, "q")) >= 0)
		switch (c) {
		case 'q':
			jid_only = 1;
			break;
		default:
			out1fmt(
			    "usage: cbsdjls [-dhNnqv] [-j jail] [param ...]");
			return 1;
		}

	/* Add the parameters to print. */
	if (jid_only == 1) {
		add_param("jid", NULL, (size_t)0, NULL, JP_USER);
		add_param("lastjid", &lastjid, sizeof(lastjid), NULL, 0);
		/* Fetch the jail(s) and print the parameters. */
		for (lastjid = 0;
		     (lastjid = print_jids(pflags, jflags)) >= 0;) {
		}

	} else {
		add_param("jid", NULL, (size_t)0, NULL, JP_USER);
		add_param("name", NULL, (size_t)0, NULL, JP_USER);
		add_param("lastjid", &lastjid, sizeof(lastjid), NULL, 0);

		/* Fetch the jail(s) and print the parameters. */
		for (lastjid = 0;
		     (lastjid = print_jail(pflags, jflags)) >= 0;) {
		}
	}

	return 0;
}

static int
add_param(const char *name, void *value, size_t valuelen,
    struct jailparam *source, unsigned flags)
{
	struct jailparam *param, *tparams;
	int i;

	static int paramlistsize;

	/* The pseudo-parameter "all" scans the list of available parameters. */
	if (!strcmp(name, "all")) {
		int tnparams = jailparam_all(&tparams);
		if (tnparams < 0) {
			out1fmt("error: %s", jail_errmsg);
			return 1;
		}
		qsort(tparams, (size_t)tnparams, sizeof(struct jailparam),
		    sort_param);
		for (i = 0; i < tnparams; i++)
			add_param(tparams[i].jp_name, NULL, (size_t)0,
			    tparams + i, flags);
		free(tparams);
		return -1;
	}

	/* Check for repeat parameters. */
	for (i = 0; i < nparams; i++)
		if (!strcmp(name, params[i].jp_name)) {
			if (value != NULL &&
			    jailparam_import_raw(params + i, value, valuelen) <
				0) {
				out1fmt("error: %s", jail_errmsg);
				return 1;
			}
			params[i].jp_flags |= flags;
			if (source != NULL)
				jailparam_free(source, 1);
			return i;
		}

	/* Make sure there is room for the new param record. */
	if (!nparams) {
		paramlistsize = 32;
		params = malloc(paramlistsize * sizeof(*params));
		param_parent = malloc(paramlistsize * sizeof(*param_parent));
		if (params == NULL || param_parent == NULL) {
			out1fmt("malloc");
			return 1;
		}
	} else if (nparams >= paramlistsize) {
		paramlistsize *= 2;
		void *tmp_params = realloc(params,
		    paramlistsize * sizeof(*params));
		void *tmp_param_parent = realloc(param_parent,
		    paramlistsize * sizeof(*param_parent));
		if (tmp_params == NULL || tmp_param_parent == NULL) {
			free(params);
			free(param_parent);
			out1fmt("realloc");
			return 1;
		} else {
			params = tmp_params;
			param_parent = tmp_param_parent;
		}
	}

	/* Look up the parameter. */
	param_parent[nparams] = -1;
	param = params + nparams++;
	if (source != NULL) {
		*param = *source;
		param->jp_flags |= flags;
		return param - params;
	}
	if (jailparam_init(param, name) < 0 ||
	    (value != NULL ? jailparam_import_raw(param, value, valuelen) :
				   jailparam_import(param, value)) < 0) {
		if (flags & JP_OPT) {
			nparams--;
			return (-1);
		}
		out1fmt("error: %s", jail_errmsg);
		return 1;
	}
	param->jp_flags = flags;
	return param - params;
}

static int
sort_param(const void *a, const void *b)
{
	const struct jailparam *parama, *paramb;
	char *ap, *bp;

	/* Put top-level parameters first. */
	parama = a;
	paramb = b;
	ap = strchr(parama->jp_name, '.');
	bp = strchr(paramb->jp_name, '.');
	if (ap && !bp)
		return (1);
	if (bp && !ap)
		return (-1);
	return (strcmp(parama->jp_name, paramb->jp_name));
}

static int
print_jail(int pflags, int jflags)
{
	int jid = jailparam_get(params, nparams, jflags);

	if (jid < 0)
		return jid;
	out1fmt("%d %s\n", *(int *)params[0].jp_value,
	    (char *)params[1].jp_value);

	return (jid);
}

static int
print_jids(int pflags, int jflags)
{
	int jid = jailparam_get(params, nparams, jflags);

	if (jid < 0)
		return jid;
	out1fmt("%d ", *(int *)params[0].jp_value);

	return (jid);
}

#endif
// DFLY
