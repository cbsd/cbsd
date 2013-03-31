#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void usage();
static int initialize_listen_socket( int pf, int af, unsigned short port );
static void child_handler( int sig );

static char* argv0;


int
main( int argc, char* argv[] )
    {
    unsigned short port;
    char** child_argv;
    int listen_fd, conn_fd;
    struct sockaddr_in sin;
    unsigned int sz;
    fd_set lfdset;
    int maxfd;

    argv0 = argv[0];

    /* Get arguments. */
    if ( argc < 3 )
	usage();
    port = (unsigned short) atoi( argv[1] );
    child_argv = argv + 2;

    /* Initialize listen socket.  If we have v6 use that, since its sockets
    ** will accept v4 connections too.  Otherwise just use v4.
    */
    listen_fd = initialize_listen_socket( PF_INET, AF_INET, port );

    /* Set up a signal handler for child reaping. */
    (void) signal( SIGCHLD, child_handler );

    for (;;)
	{
	/* Accept a new connection. */
	sz = sizeof(sin);
	conn_fd = accept( listen_fd, (struct sockaddr*) &sin, &sz );
	if ( conn_fd < 0 )
	    {
	    if ( errno == EINTR )	/* because of SIGCHLD (or ptrace) */
		continue;
	    perror( "accept" );
	    exit( 1 );
	    }

	/* Fork a sub-process. */
	if ( fork() == 0 )
	    {
	    /* Close standard descriptors and the listen socket. */
	    (void) close( 0 );
	    (void) close( 1 );
	    (void) close( 2 );
	    (void) close( listen_fd );
	    /* Dup the connection onto the standard descriptors. */
	    (void) dup2( conn_fd, 0 );
	    (void) dup2( conn_fd, 1 );
	    (void) dup2( conn_fd, 2 );
	    (void) close( conn_fd );
	    /* Run the program. */
	    (void) execv( child_argv[0], child_argv );
	    /* Something went wrong. */
	    perror( "execl" );
	    exit( 1 );
	    }
	/* Parent process. */
	(void) close( conn_fd );
	}

    }


static
void usage()
    {
    (void) fprintf( stderr, "usage:  %s port program [args...]\n", argv0 );
    exit( 1 );
    }


static void
child_handler( int sig )
    {
    pid_t pid;
    int status;

    /* Set up the signal handler again.  Don't need to do this on BSD
    ** systems, but it doesn't hurt.
    */
    (void) signal( SIGCHLD, child_handler );

    /* Reap defunct children until there aren't any more. */
    for (;;)
        {
        pid = waitpid( (pid_t) -1, &status, WNOHANG );
        if ( (int) pid == 0 )           /* none left */
            break;
        if ( (int) pid < 0 )
            {
            if ( errno == EINTR )       /* because of ptrace */
                continue;
            /* ECHILD shouldn't happen with the WNOHANG option, but with
            ** some kernels it does anyway.  Ignore it.
            */
            if ( errno != ECHILD )
                perror( "waitpid" );
            break;
            }
        }
    }


static int
initialize_listen_socket( int pf, int af, unsigned short port )
    {
    int listen_fd;
    int on;
    struct sockaddr_in sa;

    /* Create socket. */
    listen_fd = socket( pf, SOCK_STREAM, 0 );
    if ( listen_fd < 0 )
        {
	perror( "socket" );
        exit( 1 );
        }

    /* Allow reuse of local addresses. */
    on = 1;
    if ( setsockopt(
             listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on) ) < 0 )
	{
	perror( "setsockopt SO_REUSEADDR" );
	exit( 1 );
	}

    /* Set up the sockaddr. */
    (void) memset( (char*) &sa, 0, sizeof(sa) );
    sa.sin_family = af;
//    sa.sin_addr.s_addr = htonl( INADDR_ANY );
    sa.sin_addr.s_addr= inet_addr("127.0.0.1");

    sa.sin_port = htons( port );

    /* Bind it to the socket. */
    if ( bind( listen_fd, (struct sockaddr*) &sa, sizeof(sa) ) < 0 )
        {
	perror( "bind" );
        exit( 1 );
        }

    /* Start a listen going. */
    if ( listen( listen_fd, 1024 ) < 0 )
        {
	perror( "listen" );
        exit( 1 );
        }

    return listen_fd;
    }
