// Part of the CBSD project
//
#ifndef ELF_TABLES_H_
#define ELF_TABLES_H_
struct _elf_corres {
	int		elf_nb;
	const char     *string;
};

static struct _elf_corres mach_corres[] = {
	{EM_386, "x86"},
	{EM_AMD64, "x86"},
	{EM_ARM, "arm"},
	{EM_MIPS, "mips"},
	{EM_PPC, "powerpc"},
	{EM_PPC64, "powerpc"},
	{EM_SPARCV9, "sparc64"},
	{EM_IA_64, "ia64"},
	{-1, NULL},
};

static struct _elf_corres wordsize_corres[] = {
	{ELFCLASS32, "32"},
	{ELFCLASS64, "64"},
	{-1, NULL},
};

static struct _elf_corres endian_corres[] = {
	{ELFDATA2MSB, "eb"},
	{ELFDATA2LSB, "el"},
	{-1, NULL}
};

#ifndef EF_MIPS_ABI
#define EF_MIPS_ABI	0x0000f000
#endif
#define E_MIPS_ABI_O32	0x00001000
#define E_MIPS_ABI_N32	0x00000020

#define NT_VERSION	1
#define NT_ARCH	2

#define FALSE 0
#define TRUE 1

//default params
int		show_help = 0;
int		show_arch = 0;
int		show_ver = 0;
int		show_freebsdver = 0;
int		show_osname = 0;
int		show_wordsize = 0;

/* List of all elf_tables arguments */
enum {
	C_HELP,
	C_ARCH,
	C_VER,
	C_FREEBSDVER,
	C_OSNAME,
	C_WORDSIZE,
};

#endif				/* ELF_TABLES_H_ */
