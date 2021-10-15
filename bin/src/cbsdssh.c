/*
* ./ssh2_exec 127.0.0.1 22222 user password "uptime" return 0 if success 1
* if no connection 2 if no file
*/

#include <libssh2.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define MAXCMD 10000

const char     *username = "user";
const char     *password = "password";

static void
kbd_callback(const char *name, int name_len,
	     const char *instruction, int instruction_len,
	     int num_prompts,
	     const LIBSSH2_USERAUTH_KBDINT_PROMPT * prompts,
	     LIBSSH2_USERAUTH_KBDINT_RESPONSE * responses,
	     void **abstract)
{
	(void)name;
	(void)name_len;
	(void)instruction;
	(void)instruction_len;
	if (num_prompts == 1) {
		responses[0].text = strdup(password);
		responses[0].length = strlen(password);
	}
	(void)prompts;
	(void)abstract;
}				/* kbd_callback */


static int
waitsocket(int socket_fd, LIBSSH2_SESSION * session)
{
	struct timeval	timeout;
	int		rc;
	fd_set		fd;
	fd_set         *writefd = NULL;
	fd_set         *readfd = NULL;
	int		dir;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);

	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);

	if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

	return rc;
}

int
usage()
{
	printf("execute remote (via ssh) command\n");
	printf("require:\n");
	printf("opt: 192.168.0.1 port user password cmd\n\n");
	printf("return 0 if success, 1 - no connection, 2 - no file\n");
	printf("Example: cbsd cbsdssh 192.168.0.1 22 cbsd password update\n");
	exit(0);
}

int
main(int argc, char *argv[])
{
	const char	*hostname = "127.0.0.1";
	char		commandline[MAXCMD];
	int		port = 22;
	unsigned long	hostaddr;
	int		sock;
	struct sockaddr_in sin;
	const char	*fingerprint;
	LIBSSH2_SESSION *session;
	LIBSSH2_CHANNEL *channel;
	int		rc        , i;
	int		exitcode = 0;
	char		*exitsignal = (char *)"none";
	int		bytecount = 0;
	size_t		len;
	LIBSSH2_KNOWNHOSTS *nh;
	int		type;
	char		*userauthlist;
	int		auth_pw = 0;

	if (!strcmp(argv[1], "--help"))
		usage();

	if (argc > 1)
		/* must be ip address only */
		hostname = argv[1];

	if (argc > 2) {
		port = atoi(argv[2]);
	}
	if (argc > 3) {
		username = argv[3];
	}
	if (argc > 4) {
		password = argv[4];
	}
	memset(commandline, 0, sizeof(commandline));

	for (i = 5; i < argc; i++) {
		strcat(commandline, argv[i]);
		if (i + 1 != argc)
			strcat(commandline, " ");
	}

	rc = libssh2_init(0);
	if (rc != 0) {
		fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
		return 1;
	}
	hostaddr = inet_addr(hostname);

	/*
	 * Ultra basic "connect to port 22 on localhost" Your code is
	 * responsible for creating the socket establishing the connection
	 */
	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr *)(&sin),
		    sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "failed to connect!\n");
		return 1;
	}
	session = libssh2_session_init();
	if (libssh2_session_startup(session, sock)) {
		fprintf(stderr, "Failure establishing SSH session\n");
		return 1;
	}
	/*
	 * At this point we havn't authenticated, The first thing to do is
	 * check the hostkey's fingerprint against our known hosts Your app
	 * may have it hard coded, may go to a file, may present it to the
	 * user, that's your call
	 */
	fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

	/* check what authentication methods are available */
	userauthlist = libssh2_userauth_list(session, username, strlen(username));

	if (strstr(userauthlist, "password") != NULL) {
		auth_pw |= 1;
	}
	if (strstr(userauthlist, "keyboard-interactive") != NULL) {
		auth_pw |= 2;
	}
	if (strstr(userauthlist, "publickey") != NULL) {
		auth_pw |= 4;
	}
	if (auth_pw & 1) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, username, password)) {
			exitcode = 1;
			goto shutdown;
		}
	} else if (auth_pw & 2) {
		/* Or via keyboard-interactive */
		if (libssh2_userauth_keyboard_interactive(session, username,
							  &kbd_callback)) {
			exitcode = 1;
			goto shutdown;
		}
	}
#if 0
	libssh2_trace(session, ~0);
#endif

	/* Exec non-blocking on the remove host */
	while ((channel = libssh2_channel_open_session(session)) == NULL &&
	       libssh2_session_last_error(session, NULL, NULL, 0) ==
	       LIBSSH2_ERROR_EAGAIN) {
		waitsocket(sock, session);
	}
	if (channel == NULL) {
		fprintf(stderr, "Error\n");
		exit(1);
	}
	while ((rc = libssh2_channel_exec(channel, commandline)) ==
	       LIBSSH2_ERROR_EAGAIN) {
		waitsocket(sock, session);
	}
	if (rc != 0) {
		fprintf(stderr, "Error\n");
		exit(1);
	}
	for (;;) {
		/* loop until we block */
		int		rc;
		do {
			char		buffer    [0x4000];
			rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
			if (rc > 0) {
				int		i;
				bytecount += rc;
				for (i = 0; i < rc; ++i)
					fputc(buffer[i], stdout);
			}
		}
		while (rc > 0);

		/*
		 * this is due to blocking that would occur otherwise so we
		 * loop on this condition
		 */
		if (rc == LIBSSH2_ERROR_EAGAIN) {
			waitsocket(sock, session);
		} else
			break;
	}
	exitcode = 127;
	while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
		waitsocket(sock, session);

	if (rc == 0) {
		exitcode = libssh2_channel_get_exit_status(channel);
		libssh2_channel_get_exit_signal(channel, &exitsignal,
					      NULL, NULL, NULL, NULL, NULL);
	}
	libssh2_channel_free(channel);
	channel = NULL;

shutdown:

	libssh2_session_disconnect(session,
				   "Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif
	//fprintf(stderr, "all done\n");

	libssh2_exit();

	return exitcode;
}
