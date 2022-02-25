#ifndef CBSDEXTENSIONS_H
#define CBSDEXTENSIONS_H

#ifdef WITH_REDIS
#include "cbsdredis.h"
#define _REDIS(cmds) cmds
#else
#define _REDIS(cmds)
#endif

#ifdef WITH_DBI
#include "sqlcmd.h"
#define _DBI(cmds) cmds
#else
#define _DBI(cmds)
#endif

#ifdef WITH_INFLUX
#include "cbsdinflux.h"
#define _INFLUX(cmds) cmds
#else
#define _INFLUX(cmds)
#endif

#endif
