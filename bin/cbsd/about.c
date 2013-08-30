// This is implemenation of build-in "about" command
// For identification of correct CBSD shell only
#include "output.h"

int
aboutcmd(int argc, char **argv)
{
    out1fmt("CBSD Project\n");
    return 0;
}
