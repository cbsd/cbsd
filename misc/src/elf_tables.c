// Part of the CBSD Project
//
#include <stdio.h>
#include <sys/param.h>
#include <sys/elf_common.h>
#include <sys/endian.h>
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <paths.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include "elf_tables.h"

#define roundup2(x, y)	(((x)+((y)-1))&(~((y)-1)))	/* if y is powers of two */

static void
usage(void)
{
	printf("Extract and print part of elf header data\n");
	printf("  usage: elf_tables [--osname] [--ver] [--freebsdver] [--arch] [--wordzize] <path-to-file>\n");
	printf("     --osname - show OS\n");
	printf("     --ver - show version value\n");
	printf("     --freebsdver - show FreeBSD version value ( --ver / 1000000 )\n");
	printf("     --arch - show ARCH\n");
	printf("     --wordsize - show wordsize\n");
	printf("\n");
	printf("  example: elf_tables --osname --freebsdver --arch --wordsize /bin/sh\n");
	printf("\n");
	printf("hint: output order always is -> osname:ver:freebsdver:arch:wordsize\n");
	exit(0);
}


static const char *
elf_corres_to_string(struct _elf_corres *m, int e)
{
	int		i;

	for (i = 0; m[i].string != NULL; i++)
		if (m[i].elf_nb == e)
			return (m[i].string);

	return ("unknown");
}

static const char *
aeabi_parse_arm_attributes(void *data, size_t length)
{
	uint32_t	sect_len;
	uint8_t        *section = data;

#define	MOVE(len) do {            \
	assert(length >= (len));  \
	section += (len);         \
	length -= (len);          \
} while (0)

	if (length == 0 || *section != 'A')
		return (NULL);

	MOVE(1);

	/* Read the section length */
	if (length < sizeof(sect_len))
		return (NULL);

	memcpy(&sect_len, section, sizeof(sect_len));

	/*
	 * The section length should be no longer than the section it is
	 * within
	 */
	if (sect_len > length)
		return (NULL);

	MOVE(sizeof(sect_len));

	/* Skip the vendor name */
	while (length != 0) {
		if (*section == '\0')
			break;
		MOVE(1);
	}
	if (length == 0)
		return (NULL);
	MOVE(1);

	while (length != 0) {
		uint32_t	tag_length;

		switch (*section) {
		case 1:	/* Tag_File */
			MOVE(1);
			if (length < sizeof(tag_length))
				return (NULL);
			memcpy(&tag_length, section, sizeof(tag_length));
			break;
		case 2:	/* Tag_Section */
		case 3:	/* Tag_Symbol */
		default:
			return (NULL);
		}
		/* At least space for the tag and size */
		if (tag_length <= 5)
			return (NULL);
		tag_length--;
		/* Check the tag fits */
		if (tag_length > length)
			return (NULL);

#define  MOVE_TAG(len) do {           \
	assert(tag_length >= (len));  \
	MOVE(len);                    \
	tag_length -= (len);          \
} while(0)

		MOVE(sizeof(tag_length));
		tag_length -= sizeof(tag_length);

		while (tag_length != 0) {
			uint8_t		tag;

			assert(tag_length >= length);

			tag = *section;
			MOVE_TAG(1);

			/*
			 * These tag values come from:
			 * 
			 * Addenda to, and Errata in, the ABI for the ARM
			 * Architecture. Release 2.08, section 2.3.
			 */
			if (tag == 6) {	/* == Tag_CPU_arch */
				uint8_t		val;

				val = *section;
				/*
				 * We don't support values that require more
				 * than one byte.
				 */
				if (val & (1 << 7))
					return (NULL);

				/* We have an ARMv4 or ARMv5 */
				if (val <= 5)
					return ("arm");
				else	/* We have an ARMv6+ */
					return ("armv6");
			} else if (tag == 4 || tag == 5 || tag == 32 ||
				   tag == 65 || tag == 67) {
				while (*section != '\0' && length != 0)
					MOVE_TAG(1);
				if (tag_length == 0)
					return (NULL);
				/* Skip the last byte */
				MOVE_TAG(1);
			} else if ((tag >= 7 && tag <= 31) || tag == 34 ||
				   tag == 36 || tag == 38 || tag == 42 || tag == 44 ||
			 tag == 64 || tag == 66 || tag == 68 || tag == 70) {
				/* Skip the uleb128 data */
				while (*section & (1 << 7) && length != 0)
					MOVE_TAG(1);
				if (tag_length == 0)
					return (NULL);
				/* Skip the last byte */
				MOVE_TAG(1);
			} else
				return (NULL);
#undef MOVE_TAG
		}

		break;
	}
	return (NULL);
#undef MOVE
}

static int
pkg_get_myabi(char *myfile, char *dest, size_t sz)
{
	Elf            *elf;
	Elf_Data       *data;
	Elf_Note	note;
	Elf_Scn        *scn;
	char           *src, *osname;
	const char     *arch, *abi, *fpu, *endian_corres_str;
	const char     *wordsize_corres_str;
	GElf_Ehdr	elfhdr;
	GElf_Shdr	shdr;
	int		fd        , i, ret;
	uint32_t	version;
	uint32_t	freebsd_version;
	int		first_par = 0;

	version = 0;
	freebsd_version = 0;
	ret = -1;
	scn = NULL;
	abi = NULL;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		warnx("ELF library initialization failed: %s",
		      elf_errmsg(-1));
		return (-1);
	}
	if ((fd = open(myfile, O_RDONLY)) < 0) {
		warn("open()");
		return (-1);
	}
	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		ret = -1;
		warnx("elf_begin() failed: %s.", elf_errmsg(-1));
		goto cleanup;
	}
	if (gelf_getehdr(elf, &elfhdr) == NULL) {
		ret = -1;
		warn("getehdr() failed: %s.", elf_errmsg(-1));
		goto cleanup;
	}
	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		if (gelf_getshdr(scn, &shdr) != &shdr) {
			ret = -1;
			warn("getshdr() failed: %s.", elf_errmsg(-1));
			goto cleanup;
		}
		if (shdr.sh_type == SHT_NOTE)
			break;
	}

	if (scn == NULL) {
		ret = -1;
		warn("failed to get the note section");
		goto cleanup;
	}
	data = elf_getdata(scn, NULL);
	src = data->d_buf;
	for (;;) {
		memcpy(&note, src, sizeof(Elf_Note));
		src += sizeof(Elf_Note);
		if (note.n_type == NT_VERSION)
			break;
		src += note.n_namesz + note.n_descsz;
	}
	osname = src;

	src += roundup2(note.n_namesz, 4);
	if (elfhdr.e_ident[EI_DATA] == ELFDATA2MSB)
		version = be32dec(src);
	else
		version = le32dec(src);

	for (i = 0; osname[i] != '\0'; i++)
		osname[i] = (char)tolower(osname[i]);

	wordsize_corres_str = elf_corres_to_string(wordsize_corres,
					     (int)elfhdr.e_ident[EI_CLASS]);

	arch = elf_corres_to_string(mach_corres, (int)elfhdr.e_machine);

	freebsd_version = version / 100000;

	if (show_osname) {
		if (first_par)
			snprintf(dest + strlen(dest), sz - strlen(dest), ":");
		snprintf(dest, sz, "%s", osname);
		first_par++;
	}
	if (show_ver) {
		if (first_par)
			snprintf(dest + strlen(dest), sz - strlen(dest), ":");
		snprintf(dest + strlen(dest), sz - strlen(dest), "%d", version);
		first_par++;
	}
	if (show_freebsdver) {
		if (first_par)
			snprintf(dest + strlen(dest), sz - strlen(dest), ":");
		snprintf(dest + strlen(dest), sz - strlen(dest), "%d", freebsd_version);
		first_par++;
	}
	ret = 0;

	switch (elfhdr.e_machine) {
	case EM_ARM:
		endian_corres_str = elf_corres_to_string(endian_corres,
					      (int)elfhdr.e_ident[EI_DATA]);

		/* FreeBSD doesn't support the hard-float ABI yet */
		fpu = "softfp";
		if ((elfhdr.e_flags & 0xFF000000) != 0) {
			const char     *sh_name = NULL;
			size_t		shstrndx;

			/* This is an EABI file, the conformance level is set */
			abi = "eabi";
			/* Find which TARGET_ARCH we are building for. */
			elf_getshdrstrndx(elf, &shstrndx);
			while ((scn = elf_nextscn(elf, scn)) != NULL) {
				sh_name = NULL;
				if (gelf_getshdr(scn, &shdr) != &shdr) {
					scn = NULL;
					break;
				}
				sh_name = elf_strptr(elf, shstrndx,
						     shdr.sh_name);
				if (sh_name == NULL)
					continue;
				if (strcmp(".ARM.attributes", sh_name) == 0)
					break;
			}
			if (scn != NULL && sh_name != NULL) {
				data = elf_getdata(scn, NULL);
				/*
				 * Prior to FreeBSD 10.0 libelf would return
				 * NULL from elf_getdata on the
				 * .ARM.attributes section. As this was the
				 * first release to get armv6 support assume
				 * a NULL value means arm.
				 * 
				 * This assumption can be removed when 9.x is
				 * unsupported.
				 */
				if (data != NULL) {
					arch = aeabi_parse_arm_attributes(
						 data->d_buf, data->d_size);
					if (arch == NULL) {
						ret = 1;
						warn("unknown ARM ARCH");
						goto cleanup;
					}
				}
			} else {
				ret = 1;
				warn("Unable to find the .ARM.attributes "
				     "section");
				goto cleanup;
			}
		} else if (elfhdr.e_ident[EI_OSABI] != ELFOSABI_NONE) {
			/*
			 * EABI executables all have this field set to
			 * ELFOSABI_NONE, therefore it must be an oabi file.
			 */
			abi = "oabi";
		} else {
			ret = 1;
			warn("unknown ARM ABI");
			goto cleanup;
		}
		//not ready for arm
			//snprintf(dest + strlen(dest), sz - strlen(dest),
			     //":%s:%s:%s:%s:%s", arch, wordsize_corres_str,
				   //endian_corres_str, abi, fpu);
		break;
	case EM_MIPS:
		/*
		 * this is taken from binutils sources: include/elf/mips.h
		 * mapping is figured out from binutils: gas/config/tc-mips.c
		 */
		switch (elfhdr.e_flags & EF_MIPS_ABI) {
		case E_MIPS_ABI_O32:
			abi = "o32";
			break;
		case E_MIPS_ABI_N32:
			abi = "n32";
			break;
		default:
			if (elfhdr.e_ident[EI_DATA] ==
			    ELFCLASS32)
				abi = "o32";
			else if (elfhdr.e_ident[EI_DATA] ==
				 ELFCLASS64)
				abi = "n64";
			break;
		}
		endian_corres_str = elf_corres_to_string(endian_corres,
					      (int)elfhdr.e_ident[EI_DATA]);
		//not ready for mips
			//snprintf(dest + strlen(dest), sz - strlen(dest), ":%s:%s:%s:%s",
			//arch, wordsize_corres_str, endian_corres_str, abi);
		break;
	default:
		if (show_arch) {
			if (first_par)
				snprintf(dest + strlen(dest), sz - strlen(dest), ":");
			snprintf(dest + strlen(dest), sz - strlen(dest), "%s", arch);
			first_par++;
		}
		if (show_wordsize) {
			if (first_par)
				snprintf(dest + strlen(dest), sz - strlen(dest), ":");
			snprintf(dest + strlen(dest), sz - strlen(dest), "%s", wordsize_corres_str);
		}
	}

cleanup:
	if (elf != NULL)
		elf_end(elf);

	close(fd);

	printf("%s\n", dest);

	return (ret);
}


int 
main(int argc, char *argv[])
{
	char		abi       [BUFSIZ];
	char		buf       [MAXPATHLEN];
	int		win = FALSE;
	int		optcode = 0;
	int		option_index = 0, ret = 0;

	if (argc < 2)
		usage();


	static struct option long_options[] = {
		{"help", no_argument, 0, C_HELP},
		{"arch", no_argument, 0, C_ARCH},
		{"ver", no_argument, 0, C_VER},
		{"freebsdver", no_argument, 0, C_FREEBSDVER},
		{"osname", no_argument, 0, C_OSNAME},
		{"wordsize", no_argument, 0, C_WORDSIZE},
		/* End of options marker */
		{0, 0, 0, 0}
	};

	while (TRUE) {
		optcode = getopt_long(argc, argv, "h", long_options, &option_index);
		if (optcode == -1)
			break;
		int		this_option_optind = optind ? optind : 1;
		switch (optcode) {
		case C_ARCH:
			show_arch = 1;
			break;
		case C_VER:
			show_ver = 1;
			break;
		case C_FREEBSDVER:
			show_freebsdver = 1;
			break;
		case C_OSNAME:
			show_osname = 1;
			break;
		case C_WORDSIZE:
			show_wordsize = 1;
			break;
		} //case
	} //while

		memset
		(buf, 0, sizeof(buf));

	if (optind < argc)
		while (optind < argc)
			strcat(buf, argv[optind++]);

	if (strlen(buf) < 2)
		usage();

	//If no argument choose, invert all trigger into 1
		// so we show all output
		if ((!show_arch) && (!show_ver) && (!show_freebsdver) && (!show_osname) && (!show_wordsize)) {
		show_arch = 1;
		show_ver = 1;
		show_freebsdver = 1;
		show_osname = 1;
		show_wordsize = 1;
	}
	memset(abi, 0, sizeof(BUFSIZ));

	if (pkg_get_myabi(buf, abi, BUFSIZ) != 0) {
		warn("ABI Error");
		exit(1);
	}
}
