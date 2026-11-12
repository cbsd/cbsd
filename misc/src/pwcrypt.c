#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SALTSIZE 32 // from pw.c

static char const chars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./";

char *pw_pwcrypt(char * /*password*/);

int
main(int argc, char *argv[])
{

	if (argc != 2) {
		printf("Give me password\n");
		return 1;
	}

#ifndef __linux__
	if (crypt_set_format("sha512") == 0) {
		printf("error crypt_set_format\n");
		return 1;
	}
#endif

	printf("%s", pw_pwcrypt(argv[1]));

	return 0;
}

char *
pw_pwcrypt(char *password)
{
	int i;
#ifdef __linux__
	/* libxcrypt selects the hash algorithm by the salt prefix:
	 * "$6$" is SHA-512, salt is limited to 16 chars */
#define PW_SALTLEN 16
	char salt[3 + PW_SALTLEN + 1 + 1];
#else
#define PW_SALTLEN SALTSIZE
	char salt[SALTSIZE + 1];
#endif
	char *cryptpw;
	static char buf[256];

	/*
	 * Calculate a salt value
	 */
#ifdef __linux__
	memcpy(salt, "$6$", 3);
	for (i = 0; i < PW_SALTLEN; i++) {
		salt[3 + i] = chars[arc4random_uniform(sizeof(chars) - 1)];
	}
	salt[3 + PW_SALTLEN] = '$';
	salt[3 + PW_SALTLEN + 1] = '\0';
#else
	for (i = 0; i < PW_SALTLEN; i++) {
		salt[i] = chars[arc4random_uniform(sizeof(chars) - 1)];
	}
	salt[PW_SALTLEN] = '\0';
#endif
	cryptpw = crypt(password, salt);

	if (cryptpw == NULL) {
		printf("crypt(3) failure");
		exit(1);
	}
	return strcpy(buf, cryptpw);
}
