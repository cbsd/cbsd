/*-
 * Copyright (c) 1993 The Regents of the University of California
 * Copyright (c) 2003 Peter Wemm
 * Copyright (c) 2006-2019 Devin Teske <dteske@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FrauBSD: pkgcenter/depend/cputools/cputools.h 2019-02-18 19:21:12 -0800 freebsdfrau $
 */

#ifndef _CPUTOOLS_H_
#define _CPUTOOLS_H_

#include <sys/types.h>

#ifdef HAVE_MACHINE_CPUFUNC_H
#include <machine/cpufunc.h>
#else
#ifdef __amd64__
static __inline u_long
read_rflags(void)
{
	u_long	rf;

	__asm __volatile("pushfq; popq %0" : "=r" (rf));
	return (rf);
}
static __inline void
write_rflags(u_long rf)
{
	__asm __volatile("pushq %0;  popfq" : : "r" (rf));
}
#else
static __inline u_int
read_eflags(void)
{
	u_int	ef;

	__asm __volatile("pushfl; popl %0" : "=r" (ef));
	return (ef);
}
static __inline void
write_eflags(u_int ef)
{
	__asm __volatile("pushl %0; popfl" : : "r" (ef));
}
#endif /* !__amd64__ */
static __inline void
do_cpuid(u_int ax, u_int *p)
{
	__asm __volatile("cpuid"
			 : "=a" (p[0]), "=b" (p[1]), "=c" (p[2]), "=d" (p[3])
			 :  "0" (ax));
}
#endif /* !HAVE_MACHINE_CPUFUNC_H */
#ifdef HAVE_MACHINE_PSL_H
#include <machine/psl.h>
#endif
#ifdef HAVE_MACHINE_SPECIALREG_H
#include <machine/specialreg.h>
#endif

#ifndef PSL_ID
#define PSL_ID 0x00200000
#endif

#ifndef HTT_FLAG
#define HTT_FLAG 0x10000000
#endif

#ifndef VMX_FLAG
#define VMX_FLAG 0x200000
#endif

#ifndef AMDID_LM
#define AMDID_LM 0x20000000
#endif

#endif /* !_CPUTOOLS_H_ */
