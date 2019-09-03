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
__FBSDID("$FrauBSD: pkgcenter/depend/cputools/x86_64.c 2019-02-18 19:14:24 -0800 freebsdfrau $");
#endif

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cputools.h"

int
main(int argc, char *argv[])
{
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

		/* Check for vendors that support AMD features */
		if (strncmp(cpu_vendor, "GenuineIntel", 12) == 0 ||
		    strncmp(cpu_vendor, "AuthenticAMD", 12) == 0)
		{
			/* Has to support AMD features */
			do_cpuid(0x80000000, regs);
			if (regs[0] >= 0x80000001)
			{
				/* Check for long mode */
				do_cpuid(0x80000001, regs);
				has_feature = (regs[3] & AMDID_LM);
			}
		}
	}

	printf("x86_64 support: %s\n",
	    has_feature ? "YES" : "NO" );

	return (EXIT_SUCCESS);
}
