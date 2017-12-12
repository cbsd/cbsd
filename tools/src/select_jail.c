// CBSD Project 2017
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

#define BUFLEN	1024
#define MAXJNAME 128

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
	{ perror("malloc failure"); abort(); } } while(0)

struct winsize w;

struct item_data {
	int id;
	int cid;
	char name[MAXJNAME];
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

int usage()
{
	printf("Usage: select_jail <filename>\n");
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
		printf ("%s\n", curdata->name);
		curdata = curdata->next;
	}
	return 0;
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
	char buf[BUFLEN];
	int x=0;
	int pass=0;		// symbol range treshold
	int id;
	int special;
	int manual_input=0;	// if windows size less then number of elements
	int second_id=1;

	if (argc<2) usage();

	fp=fopen(argv[1],"r");

	if (!fp) {
		printf("Unable to open file %s\n",argv[1]);
		exit(2);
	}

	// get terminal size
	ioctl(0, TIOCGWINSZ, &w);

	// we have extra 'CANCEL' choice, so max_choice[0]='cancel' (and single choice before load data)
	max_choice=1;
	tmp_id=96;
	id=0;

	// load data
	while (!feof(fp)) {

		memset(buf,0,sizeof(buf));
		fscanf(fp,"%s",buf);
		if (feof(fp)) break;
		CREATE(m_item, struct item_data, 1);
		tofree = string = strdup(buf);
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

		free(tofree);

		if (x!=3) {
			printf("Warning: not <active>.int:<name>.str:<node>.str format: [%s], skipp\n",argv[1]);
		} else {

			tmp_id++;
			id++;

			//manage index here
			//first stage: a-z -> A
			if ((tmp_id>122)&&(pass==0)) {
				tmp_id=65;
				pass=1;
			}

			//second stage: A-Z -> 1
			if ((tmp_id>90)&&(pass==1)) {
				tmp_id=49;
				pass=2;
			}

			// too many jails, begin from 'a' again
			// 1-9 -> a, reset stages to zero
			if ((tmp_id>57)&&(pass==2)) {
				tmp_id=97;
				pass=3;
			}

			// loaded
			m_item->cid = tmp_id;
			m_item->id = id;
			m_item->next = item_list;
			item_list = m_item;
			max_choice++;
		}
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

				if (item==cur_choice)
					printf("%s",SELECT);

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
				printf("Cancel\n");
				exit(1);
			}
			if (is_number(buf)) {
				printf("This is jail name\n");
				//assume got jname here
				printf("%s\n",buf);
				fp=fopen(argv[1],"w");
				fputs(buf,fp);
				fclose(fp);
				exit(0);
			} else {
				printf("This is jail ID\n");
				// this is number, find jail by id
				id=atoi(buf);
				tmp_id=1;
				for ( m_item = item_list; m_item; m_item = m_item->next)
				{
					if (id == tmp_id) {
						printf("%s\n",m_item->name);
						fp=fopen(argv[1],"w");
						fputs(m_item->name,fp);
						fclose(fp);
						exit(0);
				}
				tmp_id++;
				}
				printf("Wrong input, no such jail\n");
				exit(2);
			}
		}

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

		printf("\033[%dA",max_choice+1);		//6A - 5 items + extra \n

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
	for(i=0;i<max_choice;i++) {
		printf("\n");
	}

	if (cur_choice==1) {
		// Cancel or Esc was pressed
		printf("Cancel\n");
		exit(1);
	} else {
		tmp_id=2;
		for ( m_item = item_list; m_item; m_item = m_item->next)
		{
			if (cur_choice == tmp_id) {
				printf("%s\n",m_item->name);
				fp=fopen(argv[1],"w");
				fputs(m_item->name,fp);
				fclose(fp);
				exit(0);
			}
			tmp_id++;
		}
	}

	return 1;
}
