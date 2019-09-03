/*-
 * Copyright (c) 2006-2019 Devin Teske <dteske@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FrauBSD: pkgcenter/depend/cputools/ept.c 2019-02-19 11:43:31 -0800 freebsdfrau $");
#endif

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cputools.h"

int
main(int argc, char *argv[])
{
	uint8_t cpu_type;
	uint8_t cpu_fam, cpu_extfam, cpu_family;
	uint8_t cpu_mod, cpu_extmod, cpu_model;
	uint8_t cpu_stepping;
	int has_feature = 0;
	int vendor[3];
	char *cpu_vendor;
	u_int regs[4];
#ifdef __amd64__
	register_t rflags;
#else
	u_int eflags;
#endif

	/* Check for presence of "cpuid" */
#ifdef __amd64__
	rflags = read_rflags();
	write_rflags(rflags ^ PSL_ID);
	if (((rflags ^ read_rflags()) & PSL_ID) != 0)
#else
	eflags = read_eflags();
	write_eflags(eflags ^ PSL_ID);
	if (((eflags ^ read_eflags()) & PSL_ID) != 0)
#endif
	{
		/* Fetch the vendor string */
		do_cpuid(0, regs);
		vendor[0] = regs[1]; /* %ebx */
		vendor[1] = regs[3]; /* %edx */
		vendor[2] = regs[2]; /* %ecx */
		cpu_vendor = (char *)vendor;

		/* As of the Nehalem architecture (Intel Xeon 55xx CPUs),
		 * VT-x includes the Intel Extended Page Tables (EPT). */
		if (strncmp(cpu_vendor, "GenuineIntel", 12) == 0)
		{
			/* Check for VT-x feature */
			do_cpuid(0x1, regs);
			has_feature = (regs[2] & VMX_FLAG);
			if (has_feature == 0)
				goto print_status;

			/* CPU Type and Family/Model/Stepping (FMS) */
			cpu_type = (regs[0] >> 12) & 0x3;
			cpu_fam = (regs[0] >> 8) & 0xF;
			cpu_extfam = (regs[0] >> 20) & 0xFF;
			cpu_family = cpu_fam + cpu_extfam;
			cpu_mod = (regs[0] >> 4) & 0xF;
			cpu_extmod = (regs[0] >> 16) & 0xF;
			cpu_model = cpu_mod + (uint8_t)(cpu_extmod << 4);
			cpu_stepping = regs[0] & 0xF;
			printf("Type: %02x\n", cpu_type);
			printf("Family: %02x\n", cpu_family);
			printf("Model: %02x\n", cpu_model);
			printf("Stepping: %02x\n", cpu_stepping);
		}
	}

print_status:
	printf("VT-x EPT support: %s\n",
	    has_feature ? "YES" : "NO" );

	return (EXIT_SUCCESS);
}
