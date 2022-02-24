// CBSD Project 2018
// Oleg Ginzburg <olevole@olevole.ru>
// 0.1
// Obtain ISCSI discovery result
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "simplexml.h"

struct portal_data {
	char name[1024];
	char portal[1024];
	char state[1024];
	struct portal_data *next;
};

int sid=0;
int cid=0;
int tid=0;
int level=0;

char cur_name[1024];
char cur_portal[1024];
char cur_state[1024];

int last_pid=0;

struct portal_data *portals_list = NULL;

void* handler (SimpleXmlParser parser, SimpleXmlEvent event, 
	const char* szName, const char* szAttribute, const char* szValue);
void parse (char* sData, long nDataLen);
	
void trim (const char* szInput, char* szOutput);
char* getIndent (int nDepth);
char* getReadFileDataErrorDescription (int nError);
int readFileData (char* sFileName, char** sData, long *pnDataLen);


/* memory utils **********************************************************/

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
	{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
	if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
	{ perror("realloc failure"); abort(); } } while(0)

#define REMOVE_FROM_LIST(item, head, next)      \
	if ((item) == (head))                \
		head = (item)->next;              \
	else {                               \
		temp = head;                      \
		while (temp && (temp->next != (item))) \
			temp = temp->next;             \
		if (temp)                         \
			temp->next = (item)->next;     \
	}
void trim_spaces(char *input)
{
	char *dst = input, *src = input;
	char *end;

	while (isspace((unsigned char)*src))
	{
		++src;
	}

	end = src + strlen(src) - 1;
	while (end > src && isspace((unsigned char)*end))
	{
		*end-- = 0;
	}

	if (src != dst)
	{
		while ((*dst++ = *src++));
	}
}

int list_portals()
{
	struct portal_data *ch;

	for (ch = portals_list; ch; ch = ch->next) {
		printf("NAME: %s\n",ch->name);
	}
	return 0;
}

int portal_status()
{
	struct portal_data *sch;
	int num=0;

	for (sch = portals_list; sch; sch = sch->next) {
		fprintf(stderr,"\n *** \n");
		fprintf(stderr,"Portal: %s\n",sch->portal);
		fprintf(stderr,"Portal name: %s\n",sch->name);
		fprintf(stderr,"Portal state: %s\n",sch->state);
	}

	fprintf(stderr,"\n\n");

	for (sch = portals_list; sch; sch = sch->next) {
		if ( (strlen(sch->portal)<1)||(strlen(sch->name)<1)||(strlen(sch->state)<1) ) continue;
		printf("portal%d=\"%s\"\n",num,sch->portal);
		printf("portal_name%d=\"%s\"\n",num,sch->name);
		printf("portal_state%d=\"%s\"\n",num,sch->state);
		num++;
	}

	printf("iscsi_discovery_num=\"%d\"\n",num);
	return 0;
}

int new_portal(char *portal, char *name, char *state)
{
	struct portal_data *news;

	if (strlen(portal)<1)
		return 0;

	//add socket
	CREATE(news, struct portal_data, 1);
	strcpy(news->portal,portal);
	strcpy(news->name,name);
	strcpy(news->state,state);
	news->next = portals_list;
	portals_list = news;
	fprintf(stderr," >> [portal] %d has beed added <<\n",sid);
	return 0;
}

/* ---- example xml handler */
void* handler (SimpleXmlParser parser, SimpleXmlEvent event, 
	const char* szName, const char* szAttribute, const char* szValue)
{
	static int nDepth= 0;
//	char *tmp;
	char szHandlerName[512];
	char szHandlerAttribute[512];
	char *szHandlerValue;
	//struct portal_data *news;
	//struct core_data *newc;
	//struct thread_data *newt;
//	int last_cid=0;

	szHandlerValue=malloc(512);
	memset(szHandlerValue,0,512);


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
		fprintf(stderr,"depth: %d, val: %s\n",nDepth,szHandlerName);
		fprintf(stderr, "%6li: %s add subtag (%s)\n", 
			simpleXmlGetLineNumber(parser), getIndent(nDepth), szHandlerName);
		nDepth++;
	} else if (event == ADD_CONTENT) {
		fprintf(stderr,"depth: %d, LEVEL %d, context for:[%s] [%s]\n",nDepth,level,szHandlerName, szHandlerValue);

		if (level==0) {
			if (!strcmp(szHandlerName,"portal")) {
				fprintf(stderr,"  !ROUTE LEVEL 3: PORTAL DETECTED %s\n",szHandlerValue);
					if (szHandlerValue[0] != '\0') {
						trim_spaces(szHandlerValue);
						fprintf(stderr," NEW PORTAL: [%s]\n",szHandlerValue);
						memset(cur_portal,0,sizeof(cur_portal));
						strcpy(cur_portal,szHandlerValue);
					}
			} else if (!strcmp(szHandlerName,"name")) {
				fprintf(stderr,"  !ROUTE LEVEL 3: NAME DETECTED: %s\n",szHandlerValue);
				if (szHandlerValue[0] != '\0') {
					trim_spaces(szHandlerValue);
					fprintf(stderr," NEW NAME : %s\n",szHandlerValue);
					memset(cur_name,0,sizeof(cur_name));
					if (strlen(szHandlerValue)>1) strcpy(cur_name,szHandlerValue);
				}
			} else if (!strcmp(szHandlerName,"state")) {
				fprintf(stderr,"  !ROUTE LEVEL 3: NAME STATE: %s\n",szHandlerValue);
				if (szHandlerValue[0] != '\0') {
					trim_spaces(szHandlerValue);
					fprintf(stderr," NEW STATE : %s\n",szHandlerValue);
					memset(cur_state,0,sizeof(cur_state));
					strcpy(cur_state,szHandlerValue);
				}
			}
		}

		fprintf(stderr, "%6li: %s add content to tag %s (%s)\n", 
			simpleXmlGetLineNumber(parser), getIndent(nDepth), szHandlerName, szHandlerValue);
	} else if (event == FINISH_ATTRIBUTES) {
		fprintf(stderr,"finish for attr:%s\n",szHandlerName);
		if (!strcmp(szHandlerName,"session")) {
				new_portal(cur_portal,cur_name,cur_state);
				memset(cur_portal,0,sizeof(cur_portal));
				memset(cur_name,0,sizeof(cur_name));
				memset(cur_state,0,sizeof(cur_state));
		}
		fprintf(stderr, "%6li: %s finish attributes (%s)\n", 
			simpleXmlGetLineNumber(parser), getIndent(nDepth), szHandlerName);
	} else if (event == FINISH_TAG) {
		fprintf(stderr,"finish for tag:%s\n",szHandlerName);
		if (!strcmp(szHandlerName,"session")) {
				new_portal(cur_portal,cur_name,cur_state);
				memset(cur_portal,0,sizeof(cur_portal));
				memset(cur_name,0,sizeof(cur_name));
				memset(cur_state,0,sizeof(cur_state));
		}
		fprintf(stderr, "%6li: %s finish tag (%s)\n", 
			simpleXmlGetLineNumber(parser), getIndent(nDepth), szHandlerName);
		nDepth--;
		if (level==10) {
			if (!strcmp(szHandlerName,"cpu"))
				level=0;
		}
	}

	//list_portals();
	return handler;
}

void parse (char* sData, long nDataLen) {
	SimpleXmlParser parser= simpleXmlCreateParser(sData, nDataLen);
	if (parser == NULL) {
		fprintf(stderr, "couldn't create parser");
		return;
	}
	if (simpleXmlParse(parser, handler) != 0) {
		fprintf(stderr, "parse error on line %li:\n%s\n", 
			simpleXmlGetLineNumber(parser), simpleXmlGetErrorDescription(parser));
	}
}

/* ---- helper functions */

/**
 * Copies the input to the output string.
 *
 * If the string is less than 512 characters it is
 * simply copied, otherwise the first 508 characters
 * are copied and an elipsis (...) is appended.
 *
 * @param szInput the input string.
 * @param szOutput the output string (of at least
 * 512 characters length).
 */
void trim (const char* szInput, char* szOutput) {
	int i= 0;
	while (i < 512 && szInput[i] != 0) {
		if (szInput[i] < ' ') {
			szOutput[i]= ' ';
		} else {
			szOutput[i]= szInput[i];
		}
		i++;
	}
	if (i < 512) {
		szOutput[i]= '\0';
	} else {
		szOutput[508]= '.';
		szOutput[509]= '.';
		szOutput[510]= '.';
		szOutput[511]= '\0';
	}
}

static char* szIndent= NULL;

/**
 * Returns an indent string for the specified depth.
 *
 * @param nDepth the depth.
 * @return the indent string.
 */
char* getIndent (int nDepth) {
	if (nDepth > 500) {
		nDepth= 500;
	}
	if (szIndent == NULL) {
		szIndent= malloc(1024);
	}
	memset(szIndent, ' ', nDepth * 2);
	szIndent[nDepth * 2]= '\0';
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
char* getReadFileDataErrorDescription (int nError) {
	switch (nError) {
		case READ_FILE_NO_ERROR: return "no error";
		case READ_FILE_STAT_ERROR: return "no such file";
		case READ_FILE_OPEN_ERROR: return "couldn't open file";
		case READ_FILE_OUT_OF_MEMORY: return "out of memory";
		case READ_FILE_READ_ERROR: return "couldn't read file";
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
int readFileData (char* sFileName, char** psData, long *pnDataLen) {
	struct stat fstat;
	*psData= NULL;
	*pnDataLen= 0;
	if (stat(sFileName, &fstat) == -1) {
		return READ_FILE_STAT_ERROR;
	} else {
		FILE *file= fopen(sFileName, "r");
		if (file == NULL) {
			return READ_FILE_OPEN_ERROR;
		} else {
			*psData= malloc(fstat.st_size);
			if (*psData == NULL) {
				fclose(file);
				return READ_FILE_OUT_OF_MEMORY;
			} else {
				size_t len= fread(*psData, 1, fstat.st_size, file);
				fclose(file);
				if (len != fstat.st_size) {
					free(*psData);
					*psData= NULL;
					return READ_FILE_READ_ERROR;
				}
				*pnDataLen= len;
				return READ_FILE_NO_ERROR;
			}
		}
	}	
}

int main (int nArgc, char* szArgv[]) {
	int i;
	if (nArgc < 2) {
		fprintf(stderr, "usage: %s { file }\n", szArgv[0]);
		return 1;
	}
	for (i= 1; i < nArgc; i++) {
		char* sData;
		long nDataLen;
		int nResult= readFileData(szArgv[i], &sData, &nDataLen);
		if (nResult != 0) {
			fprintf(stderr, "couldn't read %s (%s).\n", szArgv[i], 
				getReadFileDataErrorDescription(nResult));
		} else {
			parse(sData, nDataLen);
			free(sData);
		}
	}
	portal_status();
	return 0;
}

