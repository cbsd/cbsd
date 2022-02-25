#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SALTSIZE 32 // from pw.c

static char const chars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./";

char *pw_pwcrypt(char *);

int
main(int argc, char *argv[])
{

	if (argc != 2) {
		printf("Give me password\n");
		return 1;
	}

	if (crypt_set_format("sha512") == 0) {
		printf("error crypt_set_format\n");
		return 1;
	}

	printf("%s", pw_pwcrypt(argv[1]));

	return 0;
}

char *
pw_pwcrypt(char *password)
{
	int i;
	char salt[SALTSIZE + 1];
	char *cryptpw;
	static char buf[256];

	/*
	 * Calculate a salt value
	 */
	for (i = 0; i < SALTSIZE; i++)
		salt[i] = chars[arc4random_uniform(sizeof(chars) - 1)];
	salt[SALTSIZE] = '\0';
	cryptpw = crypt(password, salt);

	if (cryptpw == NULL) {
		printf("crypt(3) failure");
		exit(1);
	}
	return strcpy(buf, cryptpw);
}
