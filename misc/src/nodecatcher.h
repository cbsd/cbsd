#define SOCK_PATH	"/tmp/nodecatcher.sock"
#define ERROR_LOG	"/var/log/nodecatcher.log"
#define DEFAULT_PIDFILE_PATH	"/tmp/nodecatcher.pid"

#define DBPATH	"/var/db/nodes.sqlite"

#define MAXNODELEN 512

int debug=0;
int online=0;

// timeout for kqueue in sec. Default = 20 sec
static int interval=5;
