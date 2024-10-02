/*---------------------------------------------------------------------------*\
  NAME
	flock.c - flock(2), emulated (more or less) in terms of
                  lockf(3)

  BUGS
        Doesn't support shared locks.

  LICENSE

	This source code is released under a BSD-style. See the LICENSE
        file for details.

  Copyright (c) 2011-2015 Brian M. Clapper, bmc@clapper.org
\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\
                                 Includes
\*---------------------------------------------------------------------------*/

#include <unistd.h>
#include <errno.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include "config.h"
#include "flock.h"

/*---------------------------------------------------------------------------*\
                              Public Routines
\*---------------------------------------------------------------------------*/

int flock(int fd, int op)
{
    int cmd = 0;
    errno = 0;

    if (op & LOCK_UN)
        cmd = F_ULOCK;
    else if (op & LOCK_SH)  /* can't emulate shared lock with lockf() */
        errno = EINVAL;
    else if (op & LOCK_NB)
        cmd = F_TLOCK;
    else
        cmd = F_LOCK;

    return lockf(fd, cmd, 0);
}
