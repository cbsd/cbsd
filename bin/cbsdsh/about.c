// This is implemenation of build-in "about" command
// For identification of correct CBSD shell only
#include "output.h"

#define VERSION "10.2.0a"

int
aboutcmd(int argc, char **argv)
{
	out1fmt("CBSD Project. Version %s\n",VERSION);
	return 0;
}

int
versioncmd(int argc, char **argv)
{
	out1fmt("%s\n",VERSION);
	return 0;
}
