// CBSD Project 2018
// Oleg Ginzburg <olevole@olevole.ru>
// 0.1
// Obtain CPU topology from kern.sched.topology_spec sysctl MIB
// TODO: DEEP REFACTORING WITH DYNAMIC 'cpu count="2"' PARSER, NO MAGIC IN CODE!
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "simplexml.h"

struct socket_data {
	unsigned int parent;
	unsigned int id;
	unsigned int num;
	struct socket_data *next;
};

struct core_data {
	unsigned int parent;
	unsigned int id;
	unsigned int num;
	unsigned int socket;
	struct core_data *next;
};

struct thread_data {
	unsigned int parent;
	unsigned int id;
	unsigned int num;
	unsigned int socket;
	struct thread_data *next;
};

struct core_id_data {
	unsigned int id;
	struct core_id_data *next;
};

struct socket_id_data {
	unsigned int id;
	struct socket_id_data *next;
};

int sid = 0;
int cid = 0;
int tid = 0;
int level = 0;

int last_sid = 0;

struct socket_data *sockets_list = NULL;
struct core_data *cores_list = NULL;
struct thread_data *threads_list = NULL;
struct core_id_data *core_id_list = NULL;
struct socket_id_data *socket_id_list = NULL;

void *handler(SimpleXmlParser parser, SimpleXmlEvent event, const char *szName,
    const char *szAttribute, const char *szValue);
void parse(char *sData, long nDataLen);

void trim(const char *szInput, char *szOutput);
char *getIndent(int nDepth);
char *getReadFileDataErrorDescription(int nError);
int readFileData(char *sFileName, char **sData, long *pnDataLen);

/* memory utils **********************************************************/

#define CREATE(result, type, number)                                        \
	do {                                                                \
		if (!((result) = (type *)calloc((number), sizeof(type)))) { \
			perror("malloc failure");                           \
			abort();                                            \
		}                                                           \
	} while (0)

#define RECREATE(result, type, number)                     \
	do {                                               \
		if (!((result) = (type *)realloc((result), \
			  sizeof(type) * (number)))) {     \
			perror("realloc failure");         \
			abort();                           \
		}                                          \
	} while (0)

#define REMOVE_FROM_LIST(item, head, next)             \
	if ((item) == (head))                          \
		head = (item)->next;                   \
	else {                                         \
		temp = head;                           \
		while (temp && (temp->next != (item))) \
			temp = temp->next;             \
		if (temp)                              \
			temp->next = (item)->next;     \
	}
void
trim_spaces(char *input)
{
	char *dst = input, *src = input;
	char *end;

	while (isspace((unsigned char)*src)) {
		++src;
	}

	end = src + strlen(src) - 1;
	while (end > src && isspace((unsigned char)*end)) {
		*end-- = 0;
	}

	if (src != dst) {
		while ((*dst++ = *src++))
			;
	}
}

int
list_sockets()
{
	struct socket_data *ch;

	for (ch = sockets_list; ch; ch = ch->next) {
		printf("ID: %u\n", ch->id);
	}
	return 0;
}

int
print_cores_by_sock(int socket)
{
	struct core_data *cch;
	char tmp[256];
	char buffer[10];

	memset(tmp, 0, sizeof(tmp));

	fprintf(stderr, "\nHERE CORE\n");

	for (cch = cores_list; cch; cch = cch->next) {
		fprintf(stderr, "\nCORE::%u\n", cch->socket);
		if (cch->socket != socket)
			continue;
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%u ", cch->id);
		strcat(tmp, buffer);
	}

	printf("cores_by_socket%d=\"%s\"\n", socket, tmp);
	return 0;
}

int
print_threads_by_sock(int socket)
{
	struct thread_data *tch;
	char tmp[256];
	char buffer[10];

	memset(tmp, 0, sizeof(tmp));

	fprintf(stderr, "\nHERE\n");

	for (tch = threads_list; tch; tch = tch->next) {
		fprintf(stderr, "\n::%u\n", tch->socket);
		if (tch->socket != socket)
			continue;
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%u ", tch->id);
		strcat(tmp, buffer);
	}

	printf("threads_by_socket%d=\"%s\"\n", socket, tmp);
	return 0;
}

int
topology_status()
{
	struct socket_data *sch;
	struct core_data *cch;
	struct thread_data *tch;
	int s_max = 0;
	int c_max = 0;
	int t_max = 0;
	int core_max = 0;
	int i;

	for (sch = sockets_list; sch; sch = sch->next) {
		fprintf(stderr, "Socket ID: %u\n", sch->id);
		s_max++;
	}

	for (cch = cores_list; cch; cch = cch->next) {
		fprintf(stderr, "Core ID: %u (socket %u)\n", cch->id,
		    cch->socket);
		c_max++;
		core_max++;
	}

	for (tch = threads_list; tch; tch = tch->next) {
		fprintf(stderr, "Threads ID: %u (socket %u)\n", tch->id,
		    tch->socket);
		t_max++;
		core_max++;
	}

	printf("sockets_num=\"%d\"\n", s_max);
	printf("cores_num=\"%d\"\n", c_max);
	printf("threads_num=\"%d\"\n", t_max);
	printf("cores_max=\"%d\"\n", core_max);

	for (i = 0; i < s_max; i++) {
		print_cores_by_sock(i);
		print_threads_by_sock(i);
	}

	return 0;
}

int
new_core(int parent, int id, int socket)
{
	struct core_data *newc;

	// add core, parent = 0
	CREATE(newc, struct core_data, 1);
	newc->parent = parent;
	newc->id = id;
	newc->socket = socket;
	//	newc->socket=last_sid;
	newc->next = cores_list;
	cores_list = newc;
	fprintf(stderr, "[core] %d has beed added\n", id);
	cid++;
	return cid - 1;
}

int
new_thread(int parent, int id, int socket)
{
	struct thread_data *newt;

	// add core, parent = 0
	CREATE(newt, struct thread_data, 1);
	newt->parent = parent;
	newt->id = id;
	newt->socket = socket;
	newt->next = threads_list;
	threads_list = newt;
	fprintf(stderr, "[thread] %d has beed added\n", id);
	tid++;
	return tid - 1;
}

int
new_socket(int parent, int id)
{
	struct socket_data *news;

	// add socket
	CREATE(news, struct socket_data, 1);
	news->parent = parent;
	news->id = id;
	news->next = sockets_list;
	sockets_list = news;
	fprintf(stderr, "[socket] %d has beed added\n", sid);
	return 0;
}

int
push_core_id(int id)
{
	struct core_id_data *new_core_id;

	CREATE(new_core_id, struct core_id_data, 1);
	new_core_id->id = id;
	new_core_id->next = core_id_list;
	core_id_list = new_core_id;
	fprintf(stderr, "PUSH CORE ID for SOCKET %d: %d\n", last_sid, id);
	return 0;
}

int
pop_core_id()
{
	struct core_id_data *ch;
	struct core_id_data *temp;
	int id = 0;

	ch = core_id_list;
	if (!ch)
		return -1;

	id = ch->id;

	REMOVE_FROM_LIST(ch, core_id_list, next);
	free(ch);

	return id;
}

int
push_socket_id(int id)
{
	struct socket_id_data *new_socket_id;

	CREATE(new_socket_id, struct socket_id_data, 1);
	new_socket_id->id = id;
	new_socket_id->next = socket_id_list;
	socket_id_list = new_socket_id;
	fprintf(stderr, "PUSH SOCKET ID: %d\n", id);
	return 0;
}

int
pop_socket_id()
{
	struct socket_id_data *ch;
	struct socket_id_data *temp;
	int id = 0;

	ch = socket_id_list;
	if (!ch)
		return -1;

	id = ch->id;

	REMOVE_FROM_LIST(ch, socket_id_list, next);
	free(ch);

	return id;
}

/* ---- example xml handler */
void *
handler(SimpleXmlParser parser, SimpleXmlEvent event, const char *szName,
    const char *szAttribute, const char *szValue)
{
	static int nDepth = 0;
	char *tmp;
	char szHandlerName[32];
	char szHandlerAttribute[32];
	char *szHandlerValue;
	// struct socket_data *news;
	// struct core_data *newc;
	// struct thread_data *newt;
	int last_cid = 0;

	szHandlerValue = malloc(32);
	memset(szHandlerValue, 0, 32);

	if (szName != NULL) {
		trim(szName, szHandlerName);
	}
	if (szAttribute != NULL) {
		trim(szAttribute, szHandlerAttribute);
	}
	if (szValue != NULL) {
		trim(szValue, szHandlerValue);
	}

	if (event == ADD_SUBTAG) {
		fprintf(stderr, "depth: %d, val: %s\n", nDepth, szHandlerName);
		fprintf(stderr, "%6li: %s add subtag (%s)\n",
		    simpleXmlGetLineNumber(parser), getIndent(nDepth),
		    szHandlerName);
		nDepth++;
	} else if (event == ADD_ATTRIBUTE) {
		//		printf("attribute tag:%s %s=%s\n",szHandlerName,
		//szHandlerAttribute, szHandlerValue);
		fprintf(stderr,
		    "%6li: %s ///add attribute to tag %s ([%s]=[%s])\n",
		    simpleXmlGetLineNumber(parser), getIndent(nDepth),
		    szHandlerName, szHandlerAttribute, szHandlerValue);

		if ((!strcmp(szHandlerAttribute, "cache-level")) &&
		    (!strcmp(szHandlerValue, "3"))) {
			// attribute cache-level=3 detected: new L3 domain
			fprintf(stderr, "\n* NEW SOCKET *\n");
			new_socket(last_sid, last_sid);
			push_socket_id(last_sid);
			last_sid++;
			level = 3;
		}

		if ((!strcmp(szHandlerAttribute, "name")) &&
		    (!strcmp(szHandlerValue, "SMT"))) {
			// attribute cache-level=2 detected: new L2 domain
			fprintf(stderr, "\n       * NEW CORE, Sock %d *     \n",
			    last_sid - 1);
			// uncomment for noSMP:
			// level=10;

			// ucomment for SMP:
			last_cid = pop_core_id();
			new_core(0, last_cid, last_sid - 1);
		} else if ((!strcmp(szHandlerAttribute, "name")) &&
		    (!strcmp(szHandlerValue, "THREAD"))) {
			// attribute cache-level=2 detected: new L2 domain
			fprintf(stderr, "\n* NEW THREAD, Sock %d *\n",
			    last_sid - 1);
			last_cid = pop_core_id();
			new_thread(0, last_cid, last_sid - 1);
		} else if ((!strcmp(szHandlerAttribute, "count")) &&
		    (!strcmp(szHandlerValue, "1"))) {
			last_cid = pop_core_id() + 1;
			new_core(0, last_cid, last_sid - 1);
		}

		if ((!strcmp(szHandlerAttribute, "level")) &&
		    (!strcmp(szHandlerValue, "3"))) {
			// no cpu count="2"/SMT/THREAD Routing!
			level = 300;
		} else if ((!strcmp(szHandlerAttribute, "level")) &&
		    (!strcmp(szHandlerValue, "2"))) {
			// attribute cache-level=2 detected: new L2 domain
			fprintf(stderr,
			    "\n* !NEW LOGICAL CORE %d!, Sock: %d*\n", last_cid,
			    last_sid - 1);
			// We have "cpu count="2" and SMT/THREAD flag name
			level = 200;
		}

	} else if (event == ADD_CONTENT) {
		fprintf(stderr, "depth: %d, LEVEL %d, context for:[%s] [%s]\n",
		    nDepth, level, szHandlerName, szHandlerValue);

		if (level == 200) {
			if (!strcmp(szHandlerName, "cpu")) {
				fprintf(stderr,
				    "  !ROUTE LEVEL 10: Cores ID processed: %s, SOCKET: %d\n",
				    szHandlerValue, last_sid - 1);
				while ((tmp = strsep(&szHandlerValue, ",")) !=
				    NULL) {
					if (tmp[0] == '\0')
						break; /* XXX */
					trim_spaces(tmp);
					// comment for no SMP?
					push_core_id(atoi(tmp));
					fprintf(stderr, "\n\n\nHA: [%d]\n\n\n",
					    atoi(tmp));
					// uncomment for no SMP:
					//					last_cid=pop_core_id();
					//					new_core(0,atoi(tmp),last_sid
					//- 1);
				}
				// drop level
				//			level=9;
			}
		} else if (level == 300) {
			// no cpu count="2"/SMT/THREAD Routing!
			if (!strcmp(szHandlerName, "cpu")) {
				fprintf(stderr,
				    "  !ROUTE LEVEL 10: Cores ID processed: %s, SOCKET: %d\n",
				    szHandlerValue, last_sid - 1);
				while ((tmp = strsep(&szHandlerValue, ",")) !=
				    NULL) {
					if (tmp[0] == '\0')
						break; /* XXX */
					trim_spaces(tmp);
					// comment for no SMP?
					push_core_id(atoi(tmp));
					fprintf(stderr, "\n\n\nHA: [%d]\n\n\n",
					    atoi(tmp));
					// uncomment for no SMP:
					last_cid = pop_core_id();
					new_core(0, atoi(tmp), last_sid - 1);
				}
				// drop level
				//			level=9;
			}
		}

		fprintf(stderr, "%6li: %s add content to tag %s (%s)\n",
		    simpleXmlGetLineNumber(parser), getIndent(nDepth),
		    szHandlerName, szHandlerValue);
	} else if (event == FINISH_ATTRIBUTES) {
		fprintf(stderr, "finish for:%s\n", szHandlerName);
		fprintf(stderr, "%6li: %s finish attributes (%s)\n",
		    simpleXmlGetLineNumber(parser), getIndent(nDepth),
		    szHandlerName);
	} else if (event == FINISH_TAG) {
		fprintf(stderr, "finish for:%s\n", szHandlerName);
		fprintf(stderr, "%6li: %s finish tag (%s)\n",
		    simpleXmlGetLineNumber(parser), getIndent(nDepth),
		    szHandlerName);
		nDepth--;
		if (level == 10) {
			if (!strcmp(szHandlerName, "cpu"))
				level = 0;
		}
	}

	// list_sockets();
	return handler;
}

void
parse(char *sData, long nDataLen)
{
	SimpleXmlParser parser = simpleXmlCreateParser(sData, nDataLen);
	if (parser == NULL) {
		fprintf(stderr, "couldn't create parser");
		return;
	}
	if (simpleXmlParse(parser, handler) != 0) {
		fprintf(stderr, "parse error on line %li:\n%s\n",
		    simpleXmlGetLineNumber(parser),
		    simpleXmlGetErrorDescription(parser));
	}
}

/* ---- helper functions */

/**
 * Copies the input to the output string.
 *
 * If the string is less than 32 characters it is
 * simply copied, otherwise the first 28 characters
 * are copied and an elipsis (...) is appended.
 *
 * @param szInput the input string.
 * @param szOutput the output string (of at least
 * 32 characters length).
 */
void
trim(const char *szInput, char *szOutput)
{
	int i = 0;
	while (i < 32 && szInput[i] != 0) {
		if (szInput[i] < ' ') {
			szOutput[i] = ' ';
		} else {
			szOutput[i] = szInput[i];
		}
		i++;
	}
	if (i < 32) {
		szOutput[i] = '\0';
	} else {
		szOutput[28] = '.';
		szOutput[29] = '.';
		szOutput[30] = '.';
		szOutput[31] = '\0';
	}
}

static char *szIndent = NULL;

/**
 * Returns an indent string for the specified depth.
 *
 * @param nDepth the depth.
 * @return the indent string.
 */
char *
getIndent(int nDepth)
{
	if (nDepth > 500) {
		nDepth = 500;
	}
	if (szIndent == NULL) {
		szIndent = malloc(1024);
	}
	memset(szIndent, ' ', nDepth * 2);
	szIndent[nDepth * 2] = '\0';
	return szIndent;
}

#define READ_FILE_NO_ERROR 0
#define READ_FILE_STAT_ERROR 1
#define READ_FILE_OPEN_ERROR 2
#define READ_FILE_OUT_OF_MEMORY 3
#define READ_FILE_READ_ERROR 4

/**
 * Returns an error description for a readFileData error code.
 *
 * @param nError the error code.
 * @return the error description.
 */
char *
getReadFileDataErrorDescription(int nError)
{
	switch (nError) {
	case READ_FILE_NO_ERROR:
		return "no error";
	case READ_FILE_STAT_ERROR:
		return "no such file";
	case READ_FILE_OPEN_ERROR:
		return "couldn't open file";
	case READ_FILE_OUT_OF_MEMORY:
		return "out of memory";
	case READ_FILE_READ_ERROR:
		return "couldn't read file";
	}
	return "unknown error";
}

/**
 * Reads the complete contents of a file to a character array.
 *
 * @param sFileName the name of the file to read.
 * @param psData pointer to a character array that will be
 * allocated to read the file contents to.
 * @param pnDataLen pointer to a long that will hold the
 * number of bytes read to the character array.
 * @return 0 on success, > 0 if there was an error.
 * @see #getReadFileDataErrorDescription
 */
int
readFileData(char *sFileName, char **psData, long *pnDataLen)
{
	struct stat fstat;
	*psData = NULL;
	*pnDataLen = 0;
	if (stat(sFileName, &fstat) == -1) {
		return READ_FILE_STAT_ERROR;
	} else {
		FILE *file = fopen(sFileName, "r");
		if (file == NULL) {
			return READ_FILE_OPEN_ERROR;
		} else {
			*psData = malloc(fstat.st_size);
			if (*psData == NULL) {
				fclose(file);
				return READ_FILE_OUT_OF_MEMORY;
			} else {
				size_t len = fread(*psData, 1, fstat.st_size,
				    file);
				fclose(file);
				if (len != fstat.st_size) {
					free(*psData);
					*psData = NULL;
					return READ_FILE_READ_ERROR;
				}
				*pnDataLen = len;
				return READ_FILE_NO_ERROR;
			}
		}
	}
}

int
main(int nArgc, char *szArgv[])
{
	int i;
	if (nArgc < 2) {
		fprintf(stderr, "usage: %s { file }\n", szArgv[0]);
		return 1;
	}
	for (i = 1; i < nArgc; i++) {
		char *sData;
		long nDataLen;
		int nResult = readFileData(szArgv[i], &sData, &nDataLen);
		if (nResult != 0) {
			fprintf(stderr, "couldn't read %s (%s).\n", szArgv[i],
			    getReadFileDataErrorDescription(nResult));
		} else {
			parse(sData, nDataLen);
			free(sData);
		}
	}
	topology_status();
	return 0;
}
