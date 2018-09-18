// CBSD Project 2017-2018
// Oleg Ginzburg <olevole@olevole.ru>
// 0.1
// Todo: fast-written and confusing code with magic numbers, need to refactoring
// return 0 when jail selected
// return 1 when 'cancel' or 'esc' is pressed
// return 2 on error
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <dirent.h>

#define KEY_UP		65
#define KEY_DOWN	66
#define KEY_HOME	72
#define KEY_END		70
#define KEY_PGUP	53
#define KEY_PGDN	54
#define KEY_ESC		27

#define BOLD	"\033[1m"
#define NORMAL	"\033[0m"
#define GREEN	"\033[0;32m"
#define LGREEN	"\033[1;32m"
#define CYAN	"\033[0;36m"
#define SELECT  "\033[41m"
#define WHITE	"\033[1;37m"
#define LYELLOW "\033[1;33m"

#define BUFLEN	2048
#define MAXJNAME 128
#define MAXFNAME 1024
#define MAXFULLPATH 1024
#define MAXDESCRLEN 2048

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
	{ perror("malloc failure"); abort(); } } while(0)

struct winsize w;

struct item_data {
	int id;                // sequence
	int cid;               // letter by index for hot key
	char name[MAXFNAME];	//name of file
	//char name[MAXJNAME];
	char ext[MAXFNAME];		//extension (less size?)
	char fullpath[MAXFULLPATH];	//realpath к файлу
	char descr[MAXDESCRLEN];	//descr
	char node[MAXJNAME];
	int active;
	struct item_data *next;
};

struct item_data *item_list = NULL;

int mygetch(void)
{
	int c = 0;
	struct termios term, oterm;

	tcgetattr(0, &oterm);
	memcpy(&term, &oterm, sizeof(term));
	term.c_lflag &= ~(ICANON | ECHO);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &term);
	c = getchar();
	tcsetattr(0, TCSANOW, &oterm);
	return c;
}

int usage(char *progname)
{
	printf("Usage: %s <directory> <file-for-output>\n",progname);
	exit(1);
}

int
is_number(const char *p)
{
	int i;
	int n=0;

	for (i=0;i<strlen(p)-1;i++)
		if ((p[i]>47)&&(p[i]<58))
			continue;
		else
			n=1;

	return n;
}

int print()
{
	struct item_data *curdata = item_list;

	while (curdata != NULL) {
		printf("%s\n", curdata->name);
		curdata = curdata->next;
	}
	return 0;
}

/* qsort C-string comparison function */ 
static int compare_fun (const void *p, const void *q)
{
    const char *l= p;
    const char *r= q;
    int cmp;

    cmp= strcmp (l, r);
    return cmp;
}

void reverse() {
	// curdata traverses the list, first is reset to empty list.
	struct item_data *curdata = item_list, *nxtNode;
	item_list = NULL;

	// Until no more in list, insert current before first and advance.
	while (curdata != NULL) {
		// Need to save next node since we're changing the current.
		nxtNode = curdata->next;
		// Insert at start of new list.
		curdata->next = item_list;
		item_list = curdata;
		// Advance to next.
		curdata = nxtNode;
	}
}

int main(int argc, char **argv)
{
	int i;
	int cur_choice;
	int item=0;
	int max_choice=0;
	char tmp_id;
	struct item_data *m_item;
	char *token, *string, *tofree;
	FILE *fp;
    FILE *fo;
	char buf[BUFLEN];
    char buf2[BUFLEN];
	int x=0;
	int pass=0;		// symbol range treshold
	int id;
	int special;
	int manual_input=0;	// if windows size less then number of elements
	int second_id=1;
	DIR *dirp;
	struct dirent *dp;
	char ext[128];		//extenstion for scan
	char descrfile[MAXFULLPATH];
    char fullpath[MAXFULLPATH];
	char descr[MAXDESCRLEN];
    int listmax=0; 
    int n=0; 
    char mylist[100][MAXFNAME];
    
	if (argc<3) usage(argv[0]);
    memset(ext,0,sizeof(ext));

    if (argv[3]!=NULL) {
		strcpy(ext,argv[3]);
	} else {
		strcpy(ext,"img");
	}

	dirp = opendir(argv[1]);

	if (dirp == NULL) {
		fprintf(stderr,"Unable to opendir: %s\n",argv[1]);
		exit(1);
	}

	// get terminal size
	ioctl(0, TIOCGWINSZ, &w);

	// we have extra 'CANCEL' choice, so max_choice[0]='cancel' (and single choice before load data)
	max_choice=1;
	tmp_id=96;
	id=0;

   	descr[0]='\0';

	// load data
    while ((dp = readdir(dirp)) != NULL && listmax < sizeof mylist / sizeof mylist[0]) {
        if (dp->d_name[0]=='.') continue;
        strncpy(mylist[listmax++] , dp->d_name, MAXFNAME);
    }
    (void)closedir(dirp);
	free(dp);
    if (listmax<1) exit(0);

    qsort( mylist , listmax , sizeof(mylist[0]), compare_fun);

    for(n=0;n<listmax;n++)
    {
		memset(buf,0,sizeof(buf));
		strcpy(buf,mylist[n]);
		CREATE(m_item, struct item_data, 1);
		tofree = string = strdup(buf);
		assert(string != NULL);

		x=0;
		while ((token = strsep(&string, ".")) != NULL) {
			switch (x) {
				case 0: strcpy(m_item->name,token); break;
				case 1: strcpy(m_item->ext,token); break;
			}
			x++;
		}

		free(tofree);

		if (!strncmp(m_item->ext,ext,strlen(ext)))  {
			// пропустим latest симлинк
			if(!strncmp(m_item->name,"latest",6))
				continue;

			tmp_id++;
			id++;

			// первый проход по буквенному индексу
			// стадия: a-z -> A
			if ((tmp_id>122)&&(pass==0)) {
				tmp_id=65;
				pass=1;
			}

			// вторая стадия: A-Z -> 1
			if ((tmp_id>90)&&(pass==1)) {
				tmp_id=49;
				pass=2;
			}

			// слишком много файлов, начинаем с 'a' опять
			// сбрасываем стэдж на начало
			if ((tmp_id>57)&&(pass==2)) {
				tmp_id=97;
				pass=3;
			}

			// создаем запись
			m_item->cid = tmp_id;
			m_item->id = id;
			m_item->next = item_list;
			memset(m_item->fullpath,0,sizeof(m_item->fullpath));
			sprintf(m_item->fullpath,"%s/%s",argv[1],buf);

            m_item->id=1;

			fprintf(stderr,"Pattern file found: %s\n",buf);
            memset(fullpath,0,sizeof(fullpath));
            sprintf(fullpath,"%s/%s",argv[1],buf);
            fo=fopen(fullpath,"r");

            if (!fo) {
				fprintf(stderr,"Unable to open file %s\n",fullpath);
				break;
		    }

            memset(buf2,0,sizeof(buf2));
            fscanf(fo,"%s",buf2);
            //if (feof(fo)) break;
            tofree = string = strdup(buf2);
            assert(string != NULL);
                                        
            x=0;
            while ((token = strsep(&string, ":")) != NULL) {
                switch (x) {
                        case 0: m_item->active=atoi(token); break;
                        case 1: strcpy(m_item->name,token); break;
                        case 2: strcpy(m_item->node,token); break;
                }
                x++;
            }

            fclose(fo);

            free(tofree);

            if (x!=3) {
                fprintf(stdout,"Warning: not <active>.int:<name>.str:<node>.str format: [%s], skipp\n",fullpath);
            }

			memset(descrfile,0,MAXFULLPATH);
			sprintf(descrfile,"%s/%s.descr",argv[1],m_item->name);
			fp=fopen(descrfile,"r");
			if (fp) {
				fprintf(stderr,"Found descr!\n");
				memset(m_item->descr,0,MAXDESCRLEN);
				fgets(m_item->descr, MAXDESCRLEN, fp);
//				strcpy(m_item->descr,"OLALALLALALA BLABLA");
				fclose(fp);
				for (i=0;i<strlen(m_item->descr); i++)
					if(m_item->descr[i]=='\n') m_item->descr[i]='\0';
				fprintf(stderr,"new descr: %s descr!\n",m_item->descr);
			} else {
				(m_item->descr[0]='\0');
			}

			item_list = m_item;
			max_choice++;
		} else {
			fprintf(stderr,"[debug] warning: no %s extension (%s), skipp: [%s]\n",ext,m_item->ext,buf);
			continue;
		}
	}

	if (max_choice==1) {
		fprintf(stderr,"Files not found: %s\n",argv[1]);
		exit(1);
	}

	// reverse order
	reverse();
	//print();

	cur_choice=1;		//set on Cancel

	//-3 extra row
	if (w.ws_row-3<max_choice) {
		cur_choice=-1;
		manual_input=1;	// terminal too small for this list, input jname manually
	}

	while ( i != 10 ) {
		item=1;
		special=0;		//is special key? (started with \033, aka '[' )

		if (item==cur_choice)
			printf("%s",SELECT);

		printf(" %s0 .. CANCEL%s\n",BOLD,NORMAL);

		for ( m_item = item_list; m_item; m_item = m_item->next)
		{
			item++;
			if (m_item->active==0) {
				if (item==cur_choice) {
					printf("%s",SELECT);
					if(strlen(m_item->descr)) {
						sprintf(descr,"%s%s%s%s",BOLD,LYELLOW,m_item->descr,NORMAL);
						} else {
							memset(descr,0,MAXDESCRLEN);
						}
					}
				if (item==cur_choice) {
					if (manual_input==1)
						printf(" %s%d .. %s%s on %s%s\n",BOLD,m_item->id,GREEN,m_item->name,m_item->node,NORMAL);
					else
						printf(" %s%c .. %s on %s%s\n",SELECT,m_item->cid,m_item->name,m_item->node,NORMAL);
				} else {
					if (manual_input==1)
						printf(" %s%d .. %s%s on %s%s\n",BOLD,m_item->id,GREEN,m_item->name,m_item->node,NORMAL);
					else
						printf(" %s%c .. %s%s%s on %s%s%s\n",BOLD,m_item->cid,GREEN,m_item->name,NORMAL,LGREEN,m_item->node,NORMAL);
				}
			} else {
				if (item==cur_choice) {
					printf("%s",SELECT);
                    if (strlen(m_item->descr)) {
						sprintf(descr,"%s%s%s%s",BOLD,LYELLOW,m_item->descr,NORMAL);
					} else {
							memset(descr,0,MAXDESCRLEN);
					}
					if (manual_input==1)
						printf(" %s%d .. %s%s on %s%s\n",BOLD,m_item->id,LGREEN,m_item->name,m_item->node,NORMAL);
					else
						printf(" %s%c .. %s%s on %s%s%s\n",SELECT,m_item->cid,LGREEN,m_item->name,LGREEN,m_item->node,NORMAL);
				} else {
					if (manual_input==1)
						printf(" %s%d .. %s%s on %s%s\n",BOLD,m_item->id,LGREEN,m_item->name,m_item->node,NORMAL);
					else
						printf(" %s%c .. %s%s%s on %s%s%s\n",BOLD,m_item->cid,LGREEN,m_item->name,NORMAL,LGREEN,m_item->node,NORMAL);
				}
			}
		}

		printf("\n");

		if (manual_input==1) {
			memset(buf,0,sizeof(buf));
			printf("Enter name or ID or '0' to Cancel: ");
			fgets( buf, BUFLEN,stdin );
			if ( buf[0]=='0' ) {
				// Cancel or Esc was pressed
				fprintf(stderr,"Cancel\n");
				exit(1);
			}
			if (is_number(buf)) {
				//assume got jname here
				fprintf(stderr,"%s\n",buf);
				fp=fopen(argv[1],"w");
				fputs(buf,fp);
				fclose(fp);
				exit(0);
			} else {
				// this is number, find jail by id
				id=atoi(buf);
				tmp_id=1;
				for ( m_item = item_list; m_item; m_item = m_item->next)
				{
					if (id == tmp_id) {
						fprintf(stderr,"%s\n",m_item->name);
						fp=fopen(argv[1],"w");
						fputs(m_item->name,fp);
						fclose(fp);
						exit(0);
				}
				tmp_id++;
				}
				fprintf(stderr,"Wrong input, no such jail\n");
				exit(2);
			}
		}

        // show descr if not cancel
		if (cur_choice>1)
			printf("%s",descr);

		i = mygetch();
		if ( i == 27 ) { // if the first value is esc, [
			special = 1;
			i = mygetch(); // skip the [
			if ( i == 91 ) { //if the second value is
				i = mygetch();	// skip the [
			}
		}

		printf("'\033[1K");
		printf("\033[1000D");

		printf("\033[%dA",max_choice+1);		// items number + 1 extra \n

		if (special==1) {

		if (i==KEY_UP)
			cur_choice--;

		if (i==KEY_DOWN)
			cur_choice++;

		if ((i==KEY_PGUP)||(i==KEY_HOME))
			cur_choice=1;

		if ((i==KEY_PGDN)||(i==KEY_END))
			cur_choice=max_choice;
		}

		if (i=='0') cur_choice=1;	//jump to CANCEL

		if (cur_choice>max_choice)
			cur_choice=1;	//jump to first value after CANCEL

		if (cur_choice==0)
			cur_choice=max_choice;

		if (special==1) continue;

		//a-z 97-122
		//A-Z 65-90
		//1-9 49-57
		// in a-z + 1-9 - range
		if (special==0) {
		if ((i>96)&&(i<123)) {
			if ( i <= 95+max_choice) cur_choice=i-95;	// 96 = 'a' minus extra CANCEL
		}
		if ((i>64)&&(i<91)) {
			if ( i <= 63+max_choice) cur_choice=i-63+26;	// 96 = 'a' minus extra CANCEL + 26 symbols
		}
		if ((i>48)&&(i<58)) {
			if ( i <= 47+max_choice) cur_choice=i-47+26+26;	// 96 = 'a' minus extra CANCEL
		}
		}
	}

	//make indent after last records
	for(i=0;i<max_choice+1;i++) {
		printf("\n");
	}

	if (cur_choice==1) {
		// Cancel or Esc was pressed
		fprintf(stderr,"Cancel\n");
		exit(1);
	} else {
		tmp_id=2;
		for ( m_item = item_list; m_item; m_item = m_item->next)
		{
			if (cur_choice == tmp_id) {
				fprintf(stderr,"%s\n",m_item->name);
				fp=fopen(argv[2],"w");
				fputs(m_item->name,fp);
				fclose(fp);
				exit(0);
			}
			tmp_id++;
		}
	}

	return 1;
}
