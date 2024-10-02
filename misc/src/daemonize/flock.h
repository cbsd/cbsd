/*---------------------------------------------------------------------------*\
  NAME
	flock.c - flock(2), emulated (more or less) in terms of
                  lockf

  LICENSE

	This source code is released under a BSD-style. See the LICENSE
        file for details.

  Copyright (c) 2003-2015 Brian M. Clapper, bmc@clapper.org
\*---------------------------------------------------------------------------*/

#ifndef __DAEMONIZE_FLOCK_H__
#define __DAEMONIZE_FLOCK_H__ 1

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_UN 4
#define LOCK_NB 8

#endif /* __DAEMONIZE_FLOCK_H__ */
