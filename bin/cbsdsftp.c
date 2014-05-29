/*
 * return 0 if success 1 if no connection 2 if no file "sftp 192.168.0.1 port
 * user password /remote/file /local/file" -p|-i|-k"
 */
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

const char     *keyfile1 = "~/.ssh/id_rsa.pub";
const char     *keyfile2 = "~/.ssh/id_rsa";
const char     *username = "username";
const char     *password = "password";
const char     *sftppath = "/tmp/TEST";
const char     *localpath = "/tmp/TEST";

static void 
kbd_callback(const char *name, int name_len,
	     const char *instruction, int instruction_len, int num_prompts,
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

int 
usage()
{
	printf("secure file transfer (via ssh) program\n");
	printf("require:\n");
	printf("opt: 192.168.0.1 port user password /remote/file /local/file\n\n");
	printf("return 0 if success\n");
	printf("Example: cbsd cbsdsftp 192.168.0.1 22 cbsd password /bin/date /tmp/date\n");
	exit(0);
}


int 
main(int argc, char *argv[])
{
	unsigned long	hostaddr;
	int		sock      , i, auth_pw = 0, port = 22;
	struct sockaddr_in sin;
	const char     *fingerprint;
	char           *userauthlist;
	LIBSSH2_SESSION *session;
	int		rc;
	LIBSSH2_SFTP   *sftp_session;
	LIBSSH2_SFTP_HANDLE *sftp_handle;

	if (!strcmp(argv[1], "--help"))
		usage();

	if (argc > 1) {
		hostaddr = inet_addr(argv[1]);
	} else {
		hostaddr = htonl(0x7F000001);
	}

	if (argc > 2) {
		port = atoi(argv[2]);
	}
	if (argc > 3) {
		username = argv[3];
	}
	if (argc > 4) {
		password = argv[4];
	}
	if (argc > 5) {
		sftppath = argv[5];
	}
	if (argc > 6) {
		localpath = argv[6];
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr *)(&sin),
		    sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "failed to connect!\n");
		return -1;
	}
	session = libssh2_session_init();
	if (!session)
		return -1;

	libssh2_session_set_blocking(session, 1);

	rc = libssh2_session_startup(session, sock);
	if (rc) {
		fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
		return -1;
	}
	fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_MD5);
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
	/* if we got an 4. argument we set this option if supported */
	if (argc > 5) {
		if ((auth_pw & 1) && !strcasecmp(argv[5], "-p")) {
			auth_pw = 1;
		}
		if ((auth_pw & 2) && !strcasecmp(argv[5], "-i")) {
			auth_pw = 2;
		}
		if ((auth_pw & 4) && !strcasecmp(argv[5], "-k")) {
			auth_pw = 4;
		}
	}
	if (auth_pw & 1) {
		if (libssh2_userauth_password(session, username, password)) {
			return 1;
			goto shutdown;
		}
	} else if (auth_pw & 2) {
		if (libssh2_userauth_keyboard_interactive(session, username, &kbd_callback)) {
			return 1;
			goto shutdown;
		}
	} else if (auth_pw & 4) {
		if (libssh2_userauth_publickey_fromfile(session, username, keyfile1, keyfile2, password)) {
			printf("\tAuthentication by public key failed!\n");
			return 1;
			goto shutdown;
		}
	} else {
		printf("No supported authentication methods found!\n");
		return 1;
		goto shutdown;
	}

	sftp_session = libssh2_sftp_init(session);

	if (!sftp_session) {
		fprintf(stderr, "Unable to init SFTP session\n");
		return 1;
		goto shutdown;
	}
	sftp_handle =
		libssh2_sftp_open(sftp_session, sftppath, LIBSSH2_FXF_READ, 0);

	if (!sftp_handle) {
		return 2;
		goto shutdown;
	}
	FILE           *fp = fopen(localpath, "w");
	if (fp) {
		char		mem       [1024];
		do {
			rc = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));
			if (rc > 0) {
				fwrite(mem, rc, 1, fp);
			} else {
				break;
			}
		} while (1);
		fclose(fp);
	}
	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);

shutdown:

	libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

#ifdef WIN32
	closesocket(sock);
#else
	close(sock);
#endif
	return 0;
}
