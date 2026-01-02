// CBSD Project 2017-2025
// Oleg Ginzburg <olevole@ya.ru>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <dirent.h>

#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_HOME 72
#define KEY_END 70
#define KEY_PGUP 53
#define KEY_PGDN 54
#define KEY_ESC 27

#define BOLD "\033[1m"
#define NORMAL "\033[0m"
#define GREEN "\033[0;32m"
#define LGREEN "\033[1;32m"
#define CYAN "\033[0;36m"
#define SELECT "\033[41m"
#define WHITE "\033[1;37m"
#define LYELLOW "\033[1;33m"

struct winsize w;

int
mygetch(void)
{
	int c = 0;
	struct termios term;
	struct termios oterm;

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

int
main(int argc, char **argv)
{
	int i;
	int special=0;

	// get terminal size
	ioctl(0, TIOCGWINSZ, &w);

	i = mygetch();

	if (i == 27) { // if the first value is esc, [
		special = 1;
		i = mygetch();         // skip the [
		if (i == 91) {         // if the second value is
				i = mygetch(); // skip the [
		}
		printf("%d\n",i);
		exit(1);
	}

	printf("%c\n",i);

	exit(0);
}
