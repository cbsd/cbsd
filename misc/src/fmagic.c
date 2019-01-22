// CBSD Project 2018
// Linux dependencies: libmagic-dev
// FreeBS dependencies: -lmagic
// Wrapper for libmagic to show magic info by file
// todo1: custom group by struct/config file:
// e.g: fmagic.conf:
//   iso_type="ISO 9660 CD-ROM filesystem data|DOS/MBR boot sector'
//   bin_type=" ... | .. | ...
#include <stdio.h>
#include <magic.h>
#include <getopt.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>

#define FALSE 0
#define TRUE 1

#define MAGIC_NULL 20171006

static int      timeout = 0;

/* List of all opts */
enum {
	C_FILE,
	C_METHOD,
	C_TYPE,
};

struct mimeType {
	int mimeCode;
	const char* mimeString;
};

// Эти define-ы определены в magic.h
// Раз мы позволяем ввести пользователю нужный тип из командной строчки в виде строки,
// нужен механизм для поиска значения оригинального define по его написанию.
// Эта структура учавствует в поиске в функции magicIndex
const struct mimeType mime_info[] = {
	{ MAGIC_NONE, "MAGIC_NONE" },
	{ MAGIC_DEBUG, "MAGIC_DEBUG" },
	{ MAGIC_SYMLINK, "MAGIC_SYMLINK" },
	{ MAGIC_COMPRESS, "MAGIC_COMPRESS" },
	{ MAGIC_DEVICES, "MAGIC_DEVICES" },
	{ MAGIC_MIME_TYPE, "MAGIC_MIME_TYPE" },
	{ MAGIC_CONTINUE, "MAGIC_CONTINUE" },
	{ MAGIC_CHECK , "MAGIC_CHECK" },
	{ MAGIC_PRESERVE_ATIME, "MAGIC_PRESERVE_ATIME" },
	{ MAGIC_RAW, "MAGIC_RAW" },
	{ MAGIC_ERROR, "MAGIC_ERROR" },
	{ MAGIC_MIME_ENCODING, "MAGIC_MIME_ENCODING" },
	{ MAGIC_MIME, "MAGIC_MIME" },
	{ MAGIC_APPLE, "MAGIC_APPLE" },
	{ MAGIC_EXTENSION, "MAGIC_EXTENSION" },
	{ MAGIC_COMPRESS_TRANSP, "MAGIC_COMPRESS_TRANSP" },
	{ MAGIC_NODESC, "MAGIC_NODESC" },
	{ MAGIC_NO_CHECK_COMPRESS, "MAGIC_NO_CHECK_COMPRESS" },
	{ MAGIC_NO_CHECK_TAR, "MAGIC_NO_CHECK_TAR" },
	{ MAGIC_NO_CHECK_SOFT, "MAGIC_NO_CHECK_SOFT" },
	{ MAGIC_NO_CHECK_APPTYPE, "MAGIC_NO_CHECK_APPTYPE" },
	{ MAGIC_NO_CHECK_ELF, "MAGIC_NO_CHECK_ELF" },
	{ MAGIC_NO_CHECK_TEXT, "MAGIC_NO_CHECK_TEXT" },
	{ MAGIC_NO_CHECK_CDF, "MAGIC_NO_CHECK_CDF" },
	{ MAGIC_NO_CHECK_TOKENS, "MAGIC_NO_CHECK_TOKENS" },
	{ MAGIC_NO_CHECK_ENCODING, "MAGIC_NO_CHECK_ENCODING" },
	{ MAGIC_NULL, "NULL" },	//*special, must be last*//
};

// Возвращает значение по magic type, либо 0 если его нет
// ( MAGIC_NONE вообщем-то, тоже имеет такой код, но он нам не нужен и 
// все почти равно соответствует значению )
int magicIndex(char * magicValue) {
	int i;
	int found = FALSE;

	for(i=0; mime_info[i].mimeCode != MAGIC_NULL; i++) {
		if (!strcmp(mime_info[i].mimeString,magicValue)) {
			found = TRUE;
			break;
		}
	}
	if(found) {
		return mime_info[i].mimeCode;
	} else {
		return 0;
	}
}

static void
usage(void)
{
	printf("ECPVeil Project\n");
	printf("Wrapper for libmagic to show file type\n");
	printf("require: --file\n");
	printf("optional: --type=  [iso|bin|text], --method= \n");
	printf("If type specified, return errcode only\n");
	printf("method can be (by default: MAGIC_DEVICES):\n");
	printf("MAGIC_NONE - No flags set\n");
	printf("MAGIC_DEBUG - Turn on debugging\n");
	printf("MAGIC_SYMLINK - Follow symlinks (default for non-Windows)\n");
	printf("MAGIC_DEVICES - Look at the contents of devices\n");
	printf("MAGIC_MIME_TYPE - Return the MIME type\n");
	printf("MAGIC_CONTINUE - Return all matches (returned as an array of strings)\n");
	printf("MAGIC_CHECK - Print warnings to stderr\n");
	printf("MAGIC_PRESERVE_ATIME - Restore access time on exit\n");
	printf("MAGIC_RAW - Don't translate unprintable chars\n");
	printf("MAGIC_MIME_ENCODING - Return the MIME encoding\n");
	printf("MAGIC_MIME - (MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING)\n");
	printf("MAGIC_APPLE - Return the Apple creator and type\n");
	printf("MAGIC_NO_CHECK_TAR - Don't check for tar files\n");
	printf("MAGIC_NO_CHECK_SOFT - Don't check magic entries\n");
	printf("MAGIC_NO_CHECK_APPTYPE - Don't check application type\n");
	printf("MAGIC_NO_CHECK_ELF - Don't check for elf details\n");
	printf("MAGIC_NO_CHECK_TEXT - Don't check for text files\n");
	printf("MAGIC_NO_CHECK_CDF - Don't check for cdf files\n");
	printf("MAGIC_NO_CHECK_TOKENS - Don't check tokens\n");
	printf("MAGIC_NO_CHECK_ENCODING - Don't check text encodings\n");
	printf("\n");
	printf("example: fmagic --file=/bin/date\n");
	printf("example: fmagic --file=/etc/rc.local --type=text\n");
	printf("example: fmagic --method=MAGIC_CONTINUE --file=/bin/sh\n");
	exit(EX_USAGE);
}

int
main(int argc, char *argv[])
{
	int optcode = 0;
	int option_index = 0, ret = 0;
	char *file = NULL;
	int method = MAGIC_DEVICES;
	char *tmp_method = NULL;
	char *type = NULL;
	const char *magic_full;
	magic_t magic_cookie;

	static struct option long_options[] = {
		{"file", required_argument, 0, C_FILE},
		{"method", required_argument, 0, C_METHOD},
		{"type", required_argument, 0, C_TYPE},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	if (argc <2 )
		usage();

	while (TRUE) {
		optcode = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (optcode == -1)
			break;
		switch (optcode) {
		case C_FILE:
			file = malloc(strlen(optarg) + 1);
			memset(file, 0, strlen(optarg) + 1);
			strcpy(file, optarg);
			break;
		case C_METHOD:
			tmp_method = malloc(strlen(optarg) + 1);
			memset(tmp_method, 0, strlen(optarg) + 1);
			strcpy(tmp_method, optarg);
			method = magicIndex(tmp_method);
			if (method == 0) {
				method = MAGIC_DEVICES;
				warn("Magic index not found for %s, forcing to MAGIC_DEVICES\n",tmp_method);
				}
			free(tmp_method);
			break;
		case C_TYPE:
			file = malloc(strlen(optarg) + 1);
			memset(file, 0, strlen(optarg) + 1);
			strcpy(file, optarg);
			break;
		}
	}

	// init cookie, can be logical operand, e.g: " MAGIC_DEVICES|MAGIC_NO_CHECK_COMPRESS|MAGIC_NO_CHECK_ENCODING "
	magic_cookie = magic_open(method);
	if (magic_cookie == NULL) {
		printf("unable to initialize magic library\n");
		return 1;
	}

	// load magic database
	if (magic_load(magic_cookie, NULL) != 0) {
		printf("cannot load magic database - %s\n", magic_error(magic_cookie));
		magic_close(magic_cookie);
		return 1;
	}

	magic_full = magic_file(magic_cookie, file);
	printf("%s\n", magic_full);
	magic_close(magic_cookie);
	return 0;
}
