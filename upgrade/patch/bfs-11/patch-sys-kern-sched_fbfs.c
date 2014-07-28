--- /dev/null	2014-07-28 14:54:41.000000000 +0400
+++ sched_fbfs.c	2014-07-28 14:54:00.000000000 +0400
@@ -0,0 +1,1317 @@
+/*-
+ * Copyright (c) 1982, 1986, 1990, 1991, 1993
+ *	The Regents of the University of California.  All rights reserved.
+ * (c) UNIX System Laboratories, Inc.
+ * All or some portions of this file are derived from material licensed
+ * to the University of California by American Telephone and Telegraph
+ * Co. or Unix System Laboratories, Inc. and are reproduced herein with
+ * the permission of UNIX System Laboratories, Inc.
+ *
+ * Redistribution and use in source and binary forms, with or without
+ * modification, are permitted provided that the following conditions
+ * are met:
+ * 1. Redistributions of source code must retain the above copyright
+ *    notice, this list of conditions and the following disclaimer.
+ * 2. Redistributions in binary form must reproduce the above copyright
+ *    notice, this list of conditions and the following disclaimer in the
+ *    documentation and/or other materials provided with the distribution.
+ * 3. Neither the name of the University nor the names of its contributors
+ *    may be used to endorse or promote products derived from this software
+ *    without specific prior written permission.
+ *
+ * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
+ * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
+ * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
+ * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
+ * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
+ * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
+ * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
+ * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
+ * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
+ * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
+ * SUCH DAMAGE.
+ */
+
+#include <sys/cdefs.h>
+__FBSDID("$FreeBSD: src/sys/kern/sched_4bsd.c,v 1.131.2.7.2.1 2010/12/21 17:09:25 kensmith Exp $");
+
+#include "opt_hwpmc_hooks.h"
+#include "opt_sched.h"
+#include "opt_kdtrace.h"
+
+#include <sys/param.h>
+#include <sys/systm.h>
+#include <sys/cpuset.h>
+#include <sys/kernel.h>
+#include <sys/ktr.h>
+#include <sys/lock.h>
+#include <sys/kthread.h>
+#include <sys/mutex.h>
+#include <sys/proc.h>
+#include <sys/resourcevar.h>
+#include <sys/sched.h>
+#include <sys/sdt.h>
+#include <sys/smp.h>
+#include <sys/sysctl.h>
+#include <sys/sx.h>
+#include <sys/turnstile.h>
+#include <sys/umtx.h>
+#include <machine/pcb.h>
+#include <machine/smp.h>
+
+#ifdef HWPMC_HOOKS
+#include <sys/pmckern.h>
+#endif
+
+#ifdef KDTRACE_HOOKS
+#include <sys/dtrace_bsd.h>
+int				dtrace_vtime_active;
+dtrace_vtime_switch_func_t	dtrace_vtime_switch_func;
+#endif
+
+#define	TS_NAME_LEN (MAXCOMLEN + sizeof(" td ") + sizeof(__XSTRING(UINT_MAX)))
+
+static int realstathz = 127;
+static int sched_slice = 12;
+
+/*
+ * The time window size over which we compute the CPU utilization percentage.
+ */
+#define	PCT_WINDOW	5
+
+/*
+ * The schedulable entity that runs a context.
+ * This is  an extension to the thread structure and is tailored to
+ * the requirements of this scheduler
+ */
+struct td_sched {
+	int		ts_flags;
+	int		ts_vdeadline;	/* virtual deadline. */
+	int		ts_slice;	/* Remaining slice in number of ticks */
+	int		ts_cswtick;
+	int		ts_incrtick;
+	int		ts_used;
+	struct runq	*ts_runq;	/* runq the thread is currently on */
+#ifdef KTR
+	char		ts_name[TS_NAME_LEN];
+#endif
+};
+
+static struct cpu_group * cpu_top;
+static struct cpu_group * cpu_topology[MAXCPU];
+
+struct pcpuidlestat {
+        u_int idlecalls;
+        u_int oldidlecalls;
+};
+static DPCPU_DEFINE(struct pcpuidlestat, idlestat);
+
+/* flags kept in td_flags */
+#define TDF_DIDRUN	TDF_SCHED0	/* thread actually ran. */
+#define TDF_BOUND	TDF_SCHED1	/* Bound to one CPU. */
+#define TDF_SLICEEND    TDF_SCHED2      /* Thread time slice is over. */
+
+/* flags kept in ts_flags */
+#define	TSF_AFFINITY	0x0001		/* Has a non-"full" CPU set. */
+
+#define SKE_RUNQ_PCPU(ts)						\
+    ((ts)->ts_runq != 0 && (ts)->ts_runq != &runq)
+
+#define	THREAD_CAN_SCHED(td, cpu)	\
+    CPU_ISSET((cpu), &(td)->td_cpuset->cs_mask)
+
+static struct td_sched td_sched0;
+struct mtx sched_lock;
+
+static int	sched_tdcnt;	/* Total runnable threads in the system. */
+
+static void	setup_runqs(void);
+static void     schedcpu(void);
+static void     schedcpu_thread(void);
+static void	sched_priority(struct thread *td, u_char prio);
+static void	sched_setup(void *dummy);
+static void	sched_initticks(void *dummy);
+
+static struct	thread *edf_choose(struct rqhead * rqh);
+static struct	thread *runq_choose_bfs(struct runq * rq);
+static int 	preempt_lastcpu(struct thread *td);
+static struct	thread *worst_running_thread(void);
+
+static struct kproc_desc sched_kp = {
+        "schedcpu",
+        schedcpu_thread,
+        NULL
+};
+SYSINIT(schedcpu, SI_SUB_LAST, SI_ORDER_FIRST, kproc_start,
+    &sched_kp);
+
+SYSINIT(sched_setup, SI_SUB_RUN_QUEUE, SI_ORDER_FIRST, sched_setup, NULL);
+SYSINIT(sched_initticks, SI_SUB_CLOCKS, SI_ORDER_THIRD, sched_initticks, NULL);
+
+/*
+ * Global run queue.
+ */
+static struct runq runq;
+
+#ifdef SMP
+static cpuset_t idle_cpus_mask;
+#endif
+
+/*
+ * Priority ratios for virtual deadline per nice value calculations.
+ */
+static int prio_ratios[PRIO_MAX - PRIO_MIN + 1];
+
+SDT_PROVIDER_DEFINE(sched);
+
+SDT_PROBE_DEFINE3(sched, , , change__pri, "struct thread *",
+    "struct proc *", "uint8_t");
+SDT_PROBE_DEFINE3(sched, , , dequeue, "struct thread *",
+    "struct proc *", "void *");
+SDT_PROBE_DEFINE4(sched, , , enqueue, "struct thread *",
+    "struct proc *", "void *", "int");
+SDT_PROBE_DEFINE4(sched, , , lend__pri, "struct thread *",
+    "struct proc *", "uint8_t", "struct thread *");
+SDT_PROBE_DEFINE2(sched, , , load__change, "int", "int");
+SDT_PROBE_DEFINE2(sched, , , off__cpu, "struct thread *",
+    "struct proc *");
+SDT_PROBE_DEFINE(sched, , , on__cpu);
+SDT_PROBE_DEFINE(sched, , , remain__cpu);
+SDT_PROBE_DEFINE2(sched, , , surrender, "struct thread *",
+    "struct proc *");
+
+static void
+setup_runqs(void)
+{
+	runq_init(&runq);
+}
+
+/*
+ * Recompute process used and tick, every hz/PCT_WINDOW ticks.
+ */
+/* ARGSUSED */
+static void
+schedcpu(void)
+{
+        struct thread *td;
+        struct proc *p;
+        struct td_sched *ts;
+
+        sx_slock(&allproc_lock);
+        FOREACH_PROC_IN_SYSTEM(p) {
+                PROC_LOCK(p);
+                if (p->p_state == PRS_NEW) {
+                        PROC_UNLOCK(p);
+                        continue;
+                }
+                FOREACH_THREAD_IN_PROC(p, td) {
+                        thread_lock(td);
+                        ts = td->td_sched;
+                	if (ts->ts_incrtick == ticks){
+				thread_unlock(td);
+                                continue;
+			}
+			if (ts->ts_used < (hz / PCT_WINDOW)) {
+				ts->ts_used += 1;
+				ts->ts_incrtick = ticks;
+			}
+			thread_unlock(td);
+                }
+    		PROC_UNLOCK(p);
+	}
+	sx_sunlock(&allproc_lock);
+}
+
+/*
+ * Main loop for a kthread that executes schedcpu once a second.
+ */
+static void
+schedcpu_thread(void)
+{
+
+        for (;;) {
+                schedcpu();
+                pause("-", hz/PCT_WINDOW);
+        }
+}
+
+static int
+sysctl_kern_quantum(SYSCTL_HANDLER_ARGS)
+{
+        int error, new_val, period;
+
+        period = 1000000 / realstathz;
+        new_val = period * sched_slice;
+        error = sysctl_handle_int(oidp, &new_val, 0, req);
+        if (error != 0 || req->newptr == NULL)
+                return (error);
+        if (new_val <= 0)
+                return (EINVAL);
+        sched_slice = imax(1, (new_val + period / 2) / period);
+        hogticks = imax(1, (2 * hz * sched_slice + realstathz / 2) /
+    	    realstathz);
+        return (0);
+}
+
+SYSCTL_NODE(_kern, OID_AUTO, sched, CTLFLAG_RD, 0, "Scheduler");
+
+SYSCTL_STRING(_kern_sched, OID_AUTO, name, CTLFLAG_RD, "FBFS", 0,
+	"Scheduler name");
+
+SYSCTL_PROC(_kern_sched, OID_AUTO, quantum, CTLTYPE_INT | CTLFLAG_RW,
+	NULL, 0, sysctl_kern_quantum, "I",
+	"Length of time granted to timeshare threads in microseconds");
+
+SYSCTL_INT(_kern_sched, OID_AUTO, slice, CTLFLAG_RW, &sched_slice, 0,
+	"Length of time granted to timeshare threads in stathz ticks");
+
+static __inline void
+sched_load_add(void)
+{
+
+	sched_tdcnt++;
+	KTR_COUNTER0(KTR_SCHED, "load", "global load", sched_tdcnt);
+	SDT_PROBE2(sched, , , load__change, NOCPU, sched_tdcnt);
+}
+
+static __inline void
+sched_load_rem(void)
+{
+
+	sched_tdcnt--;
+	KTR_COUNTER0(KTR_SCHED, "load", "global load", sched_tdcnt);
+	SDT_PROBE2(sched, , , load__change, NOCPU, sched_tdcnt);
+}
+
+int
+maybe_preempt(struct thread *td)
+{
+	return (0);
+}
+
+/* I keep it here because the top command wants it. */
+static fixpt_t  ccpu = 0;
+SYSCTL_INT(_kern, OID_AUTO, ccpu, CTLFLAG_RD, &ccpu, 0, "");
+
+/* ARGSUSED */
+static void
+sched_setup(void *dummy)
+{
+	int i;
+
+	cpu_top = smp_topo();
+	CPU_FOREACH(i) {
+		cpu_topology[i] = smp_topo_find(cpu_top, i);
+		if (cpu_topology[i] == NULL)
+			panic("Can't find cpu group for %d\n", i);
+	}
+
+	prio_ratios[0] = 128;
+	for (i = 1; i <= (PRIO_MAX - PRIO_MIN); ++i) {
+		prio_ratios[i] = prio_ratios[i - 1] * 11 / 10;
+	}
+
+	setup_runqs();
+
+	/* Account for thread0. */
+	sched_load_add();
+}
+
+static void
+sched_initticks(void *dummy)
+{
+	realstathz = stathz ? stathz : hz;
+	sched_slice = realstathz / 10;  /* ~100ms */
+	hogticks = imax(1, (2 * hz * sched_slice + realstathz / 2) / realstathz);
+}
+
+/* External interfaces start here */
+
+/*
+ * Very early in the boot some setup of scheduler-specific
+ * parts of proc0 and of some scheduler resources needs to be done.
+ * Called from:
+ *  proc0_init()
+ */
+void
+schedinit(void)
+{
+	/*
+	 * Set up the scheduler specific parts of proc0.
+	 */
+	proc0.p_sched = NULL; /* XXX */
+	thread0.td_sched = &td_sched0;
+	thread0.td_lock = &sched_lock;
+	td_sched0.ts_used = 0;
+	td_sched0.ts_slice = sched_slice;
+	mtx_init(&sched_lock, "sched lock", NULL, MTX_SPIN | MTX_RECURSE);
+}
+
+int
+sched_runnable(void)
+{
+	return runq_check(&runq);
+}
+
+int
+sched_rr_interval(void)
+{
+	/* Convert sched_slice from stathz to hz. */
+	return (imax(1, (sched_slice * hz + realstathz / 2) / realstathz));
+}
+
+void
+sched_clock(struct thread *td)
+{
+	struct pcpuidlestat *stat;
+	struct td_sched *ts;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	ts = td->td_sched;
+
+	if (--ts->ts_slice > 0)
+		return;
+
+	ts->ts_vdeadline = ticks + sched_slice *
+	    prio_ratios[td->td_proc->p_nice - PRIO_MIN] / 128;
+	ts->ts_slice = sched_slice;
+	td->td_flags |= TDF_NEEDRESCHED | TDF_SLICEEND;
+	
+	CTR4(KTR_SCHED, "timeslice fill: t: %d, i: %d, r: %d, d: %d",
+	    ticks, td->td_proc->p_nice - PRIO_MIN,
+	    prio_ratios[td->td_proc->p_nice - PRIO_MIN],
+	    ts->ts_vdeadline
+	);
+		
+	CTR1(KTR_SCHED, "queue number: %d", td->td_rqindex);
+	CTR1(KTR_SCHED, "thread: 0x%x", td);
+
+        stat = DPCPU_PTR(idlestat);
+        stat->oldidlecalls = stat->idlecalls;
+        stat->idlecalls = 0;
+}
+
+/*
+ * Charge child's scheduling CPU usage to parent.
+ */
+void
+sched_exit(struct proc *p, struct thread *td)
+{
+	KTR_STATE1(KTR_SCHED, "thread", sched_tdname(td), "proc exit",
+	    "prio:%d", td->td_priority);
+
+	PROC_LOCK_ASSERT(p, MA_OWNED);
+	sched_exit_thread(FIRST_THREAD_IN_PROC(p), td);
+}
+
+void
+sched_exit_thread(struct thread *td, struct thread *child)
+{
+	KTR_STATE1(KTR_SCHED, "thread", sched_tdname(child), "exit",
+	    "prio:%d", child->td_priority);
+	thread_lock(child);
+	if ((child->td_flags & TDF_NOLOAD) == 0)
+		sched_load_rem();
+	thread_unlock(child);
+}
+
+void
+sched_fork(struct thread *td, struct thread *childtd)
+{
+	sched_fork_thread(td, childtd);
+}
+
+void
+sched_fork_thread(struct thread *td, struct thread *childtd)
+{
+	struct td_sched *ts;
+
+	childtd->td_lock = &sched_lock;
+	childtd->td_cpuset = cpuset_ref(td->td_cpuset);
+	childtd->td_priority = childtd->td_base_pri;
+	ts = childtd->td_sched;
+	bzero(ts, sizeof(*ts));
+	td->td_sched->ts_slice /= 2;
+	ts->ts_flags |= (td->td_sched->ts_flags & TSF_AFFINITY);
+	ts->ts_vdeadline = td->td_sched->ts_vdeadline;
+	ts->ts_slice = td->td_sched->ts_slice;
+//	ts->ts_slice = 1;
+	ts->ts_used = td->td_sched->ts_used;
+}
+
+void
+sched_nice(struct proc *p, int nice)
+{
+	PROC_LOCK_ASSERT(p, MA_OWNED);
+	p->p_nice = nice;
+}
+
+void
+sched_class(struct thread *td, int class)
+{
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	td->td_pri_class = class;
+}
+
+/*
+ * Adjust the priority of a thread.
+ */
+static void
+sched_priority(struct thread *td, u_char prio)
+{
+	KTR_POINT3(KTR_SCHED, "thread", sched_tdname(td), "priority change",
+	    "prio:%d", td->td_priority, "new prio:%d", prio, KTR_ATTR_LINKED,
+	    sched_tdname(curthread));
+	SDT_PROBE3(sched, , , change__pri, td, td->td_proc, prio);
+	if (td != curthread && prio > td->td_priority) {
+		KTR_POINT3(KTR_SCHED, "thread", sched_tdname(curthread),
+		    "lend prio", "prio:%d", td->td_priority, "new prio:%d",
+		    prio, KTR_ATTR_LINKED, sched_tdname(td));
+		SDT_PROBE4(sched, , , lend__pri, td, td->td_proc, prio,
+                    curthread);
+	}
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	if (td->td_priority == prio)
+		return;
+	td->td_priority = prio;
+	if (TD_ON_RUNQ(td) && td->td_rqindex != (prio / RQ_PPQ)) {
+		sched_rem(td);
+		sched_add(td, SRQ_BORING);
+	}
+}
+
+/*
+ * Update a thread's priority when it is lent another thread's
+ * priority.
+ */
+void
+sched_lend_prio(struct thread *td, u_char prio)
+{
+
+	td->td_flags |= TDF_BORROWING;
+	sched_priority(td, prio);
+}
+
+/*
+ * Restore a thread's priority when priority propagation is
+ * over.  The prio argument is the minimum priority the thread
+ * needs to have to satisfy other possible priority lending
+ * requests.  If the thread's regulary priority is less
+ * important than prio the thread will keep a priority boost
+ * of prio.
+ */
+void
+sched_unlend_prio(struct thread *td, u_char prio)
+{
+	u_char base_pri;
+
+	if (td->td_base_pri >= PRI_MIN_TIMESHARE &&
+	    td->td_base_pri <= PRI_MAX_TIMESHARE)
+		base_pri = td->td_user_pri;
+	else
+		base_pri = td->td_base_pri;
+	if (prio >= base_pri) {
+		td->td_flags &= ~TDF_BORROWING;
+		sched_prio(td, base_pri);
+	} else
+		sched_lend_prio(td, prio);
+}
+
+void
+sched_prio(struct thread *td, u_char prio)
+{
+	u_char oldprio;
+
+	/* First, update the base priority. */
+	td->td_base_pri = prio;
+
+	/*
+	 * If the thread is borrowing another thread's priority, don't ever
+	 * lower the priority.
+	 */
+	if (td->td_flags & TDF_BORROWING && td->td_priority < prio)
+		return;
+
+	/* Change the real priority. */
+	oldprio = td->td_priority;
+	sched_priority(td, prio);
+
+	/*
+	 * If the thread is on a turnstile, then let the turnstile update
+	 * its state.
+	 */
+	if (TD_ON_LOCK(td) && oldprio != prio)
+		turnstile_adjust(td, oldprio);
+}
+
+void
+sched_user_prio(struct thread *td, u_char prio)
+{
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	td->td_base_user_pri = prio;
+	if (td->td_flags & TDF_NEEDRESCHED && td->td_user_pri <= prio)
+		return;
+	td->td_user_pri = prio;
+}
+
+void
+sched_lend_user_prio(struct thread *td, u_char prio)
+{
+//	u_char oldprio;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+//	if (td->td_priority > td->td_user_pri)
+//	    sched_prio(td, td->td_user_pri);
+	    td->td_user_pri = prio;
+//	else if (td->td_priority != td->td_user_pri)
+		td->td_flags |= TDF_NEEDRESCHED;
+//	oldprio = td->td_user_pri;
+//	td->td_user_pri = prio;
+}
+
+void
+sched_sleep(struct thread *td, int pri)
+{
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	td->td_slptick = ticks;
+	if (pri)
+		sched_prio(td, pri);
+	if (TD_IS_SUSPENDED(td) || pri >= PSOCK)
+		td->td_flags |= TDF_CANSWAP;
+}
+
+void
+sched_switch(struct thread *td, struct thread *newtd, int flags)
+{
+	struct mtx *tmtx = NULL;
+	struct td_sched *ts;
+	int time_passed;
+//	int preempted;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+
+	ts = td->td_sched;
+
+	/* 
+	 * Switch to the sched lock to fix things up and pick
+	 * a new thread.
+	 * Block the td_lock in order to avoid breaking the critical path.
+	 */
+	if (td->td_lock != &sched_lock) {
+		mtx_lock_spin(&sched_lock);
+		tmtx = thread_lock_block(td);
+	} else {
+		tmtx = td->td_lock;
+	}
+
+	if ((td->td_flags & TDF_NOLOAD) == 0)
+		sched_load_rem();
+
+	if (newtd) {
+		MPASS(newtd->td_lock == &sched_lock);
+		newtd->td_flags |= (td->td_flags & TDF_NEEDRESCHED);
+	}
+
+	td->td_lastcpu = td->td_oncpu;
+//	preempted = !(td->td_flags & TDF_SLICEEND);
+//	td->td_flags &= ~TDF_NEEDRESCHED;
+	td->td_flags &= ~(TDF_NEEDRESCHED | TDF_SLICEEND);
+	td->td_owepreempt = 0;
+	td->td_oncpu = NOCPU;
+
+	/*
+	 * At the last moment, if this thread is still marked RUNNING,
+	 * then put it back on the run queue as it has not been suspended
+	 * or stopped or any thing else similar.  We never put the idle
+	 * threads on the run queue, however.
+	 */
+	if (td->td_flags & TDF_IDLETD) {
+		TD_SET_CAN_RUN(td);
+#ifdef SMP
+		CPU_CLR(PCPU_GET(cpuid), &idle_cpus_mask);
+#endif
+	} else {
+		if (TD_IS_RUNNING(td)) {
+			/* Put us back on the run queue. */
+			sched_add(td, (flags & SW_PREEMPT) ?
+//			sched_add(td, preempted ?
+			    SRQ_OURSELF|SRQ_YIELDING|SRQ_PREEMPTED :
+			    SRQ_OURSELF|SRQ_YIELDING);
+		}
+	}
+	if (newtd) {
+		/*
+		 * The thread we are about to run needs to be counted
+		 * as if it had been added to the run queue and selected.
+		 * It came from:
+		 * * A preemption
+		 * * An upcall
+		 * * A followon
+		 */
+		KASSERT((newtd->td_inhibitors == 0),
+			("trying to run inhibited thread"));
+		newtd->td_flags |= TDF_DIDRUN;
+        	TD_SET_RUNNING(newtd);
+		if ((newtd->td_flags & TDF_NOLOAD) == 0)
+			sched_load_add();
+	} else {
+		newtd = choosethread();
+		MPASS(newtd->td_lock == &sched_lock);
+	}
+
+	if (td != newtd) {
+#ifdef	HWPMC_HOOKS
+		if (PMC_PROC_IS_USING_PMCS(td->td_proc))
+			PMC_SWITCH_CONTEXT(td, PMC_FN_CSW_OUT);
+#endif
+
+		SDT_PROBE2(sched, , , off__cpu, newtd, newtd->td_proc);
+		
+                /* I feel sleepy */
+		lock_profile_release_lock(&sched_lock.lock_object);
+#ifdef KDTRACE_HOOKS
+		/*
+		 * If DTrace has set the active vtime enum to anything
+		 * other than INACTIVE (0), then it should have set the
+		 * function to call.
+		 */
+		if (dtrace_vtime_active)
+			(*dtrace_vtime_switch_func)(newtd);
+#endif
+
+		ts->ts_cswtick = ticks;
+		cpu_switch(td, newtd, tmtx);
+		lock_profile_obtain_lock_success(&sched_lock.lock_object,
+		    0, 0, __FILE__, __LINE__);
+		/*
+		 * Where am I?  What year is it?
+		 * We are in the same thread that went to sleep above,
+		 * but any amount of time may have passed. All our context
+		 * will still be available as will local variables.
+		 * PCPU values however may have changed as we may have
+		 * changed CPU so don't trust cached values of them.
+		 * New threads will go to fork_exit() instead of here
+		 * so if you change things here you may need to change
+		 * things there too.
+		 *
+		 * If the thread above was exiting it will never wake
+		 * up again here, so either it has saved everything it
+		 * needed to, or the thread_wait() or wait() will
+		 * need to reap it.
+		 */
+		time_passed = ticks - ts->ts_cswtick;
+		ts->ts_used = imax(ts->ts_used - time_passed, 0);
+		if (ts->ts_used < 0) panic("Negative ts_used value\n");
+		
+		SDT_PROBE0(sched, , , on__cpu);
+#ifdef	HWPMC_HOOKS
+		if (PMC_PROC_IS_USING_PMCS(td->td_proc))
+			PMC_SWITCH_CONTEXT(td, PMC_FN_CSW_IN);
+#endif
+	} else
+		SDT_PROBE0(sched, , , remain__cpu);
+
+#ifdef SMP
+	if (td->td_flags & TDF_IDLETD)
+		CPU_SET(PCPU_GET(cpuid), &idle_cpus_mask);
+#endif
+	sched_lock.mtx_lock = (uintptr_t)td;
+	td->td_oncpu = PCPU_GET(cpuid);
+	MPASS(td->td_lock == &sched_lock);
+}
+
+int
+preempt_lastcpu(struct thread *td)
+{
+	int cpri;
+	struct pcpu * pcpu;
+	struct td_sched *ts;
+	struct td_sched *tsc;
+	struct thread *pcpu_thr;
+	u_char c;
+
+	c = td->td_lastcpu;
+	if (c == NOCPU)
+		return (0);
+	pcpu = pcpu_find(c);
+	pcpu_thr = pcpu->pc_curthread;
+	if (pcpu_thr == NULL)
+		return (0);
+	if (pcpu_thr == pcpu->pc_idlethread) {
+		pcpu_thr->td_flags |= TDF_NEEDRESCHED;
+		if (PCPU_GET(cpuid) != c) 
+			ipi_cpu(c, IPI_AST);
+		return (1);
+	}
+	cpri = pcpu_thr->td_priority;
+	if (cpri < td->td_priority)
+		return (0);
+	if (cpri > td->td_priority) {
+		pcpu_thr->td_flags |= TDF_NEEDRESCHED;
+		if (PCPU_GET(cpuid) != c) 
+			ipi_cpu(c, IPI_AST);
+		return (1);
+	}
+	ts = td->td_sched;
+	tsc = pcpu_thr->td_sched;
+	if ((td->td_pri_class == PRI_TIMESHARE) ||
+	    (td->td_pri_class == PRI_IDLE)) {
+		if (ts->ts_vdeadline >= tsc->ts_vdeadline)
+			return (0);
+//		else
+//			return (1);
+	} else
+		return (0);
+	/*
+	 * Here, the priorities of td, and current thread on td_lastcpu are
+	 * equal. And their scheduling class is PRI_IDLE or PRI_TIMESHARE
+	 * Further, the virtual deadline of td is lower. Therefore we
+	 * reschedule the td_lastcpu.
+	 */
+	pcpu_thr->td_flags |= TDF_NEEDRESCHED;
+	if (PCPU_GET(cpuid) != c) 
+		ipi_cpu(c, IPI_AST);
+
+	return (1);
+}
+
+struct thread *
+worst_running_thread(void)
+{
+	struct td_sched *ts, *ts2;
+	struct thread *max_thread, *cthr;
+	struct pcpu *pc;
+	u_char max_prio;
+
+	max_thread = curthread;
+	MPASS(max_thread != NULL);
+	max_prio = max_thread->td_priority;
+	ts = max_thread->td_sched;
+	STAILQ_FOREACH(pc, &cpuhead, pc_allcpu) {
+		cthr = pc->pc_curthread;
+		if (cthr == NULL) {
+			continue;
+		}
+		if (max_prio < cthr->td_priority) {
+			max_thread = cthr;
+			max_prio = max_thread->td_priority;
+			ts = max_thread->td_sched;
+		} else if (max_prio == cthr->td_priority) {
+			ts2 = cthr->td_sched;
+			if (ts->ts_vdeadline > ts2->ts_vdeadline) {
+				max_thread = cthr;
+				ts = ts2;
+			}
+		}
+	}
+	MPASS(max_thread != NULL);
+	return (max_thread);
+}
+
+void
+sched_wakeup(struct thread *td)
+{
+	struct td_sched *ts;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	ts = td->td_sched;
+	td->td_flags &= ~TDF_CANSWAP;
+	td->td_slptick = 0;
+	ts->ts_slice = sched_slice;
+//	ts->ts_slice = 0;
+	sched_add(td, SRQ_BORING);
+}
+
+void
+sched_add(struct thread *td, int flags)
+{
+	struct td_sched *ts;
+	struct thread *thr_worst;
+	cpuset_t dontuse, map, map_oth;
+	u_int me;
+	struct cpu_group *cg;
+	u_char c;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	ts = td->td_sched;
+	KASSERT((td->td_inhibitors == 0),
+	    ("sched_add: trying to run inhibited thread"));
+	KASSERT((TD_CAN_RUN(td) || TD_IS_RUNNING(td)),
+	    ("sched_add: bad thread state"));
+	KASSERT(td->td_flags & TDF_INMEM,
+	    ("sched_add: thread swapped out"));
+	KTR_STATE2(KTR_SCHED, "thread", sched_tdname(td), "runq add",
+	    "prio:%d", td->td_priority, KTR_ATTR_LINKED,
+	    sched_tdname(curthread));
+	KTR_POINT1(KTR_SCHED, "thread", sched_tdname(curthread), "wokeup",
+	    KTR_ATTR_LINKED, sched_tdname(td));
+	SDT_PROBE4(sched, , , enqueue, td, td->td_proc, NULL,
+            flags & SRQ_PREEMPTED);
+
+	/*
+	 * Now that the thread is moving to the run-queue, set the lock
+	 * to the scheduler's lock.
+	 */
+	if (td->td_lock != &sched_lock) {
+		mtx_lock_spin(&sched_lock);
+		thread_lock_set(td, &sched_lock);
+	}
+	TD_SET_RUNQ(td);
+	CTR2(KTR_RUNQ, "sched_add: adding td_sched:%p (td:%p) to runq", ts, td);
+	ts->ts_runq = &runq;
+
+	if ((td->td_flags & TDF_NOLOAD) == 0)
+		sched_load_add();
+	runq_add(ts->ts_runq, td, flags);
+
+	me = PCPU_GET(cpuid);
+
+	CPU_SETOF(me, &dontuse);
+	CPU_OR(&dontuse, &stopped_cpus);
+	CPU_OR(&dontuse, &hlt_cpus_mask);
+	map = idle_cpus_mask;
+	CPU_NAND(&map, &dontuse);
+
+	/*
+	 * Firstly check if we should reschedule the last cpu the thread
+	 * run on.
+	 */
+	if (preempt_lastcpu(td)) {
+		if (!CPU_EMPTY(&map))
+			ipi_selected(map, IPI_AST);
+		return;
+	}
+	/*
+	 * Is there any idle cpu ?
+	 */
+	if (!CPU_EMPTY(&map)) {
+		cg = cpu_topology[td->td_lastcpu];
+		map_oth = map;
+		CPU_AND(&map_oth, &cg->cg_mask);
+		while ((cg != NULL) && (CPU_EMPTY(&map_oth)))
+			cg = cg->cg_parent;
+		if (!CPU_EMPTY(&map_oth)) {
+			ipi_selected(map_oth, IPI_AST);
+			return;
+		}
+		ipi_selected(map, IPI_AST);
+		return;
+	}
+	/*
+	 * We did not wake lastcpu and there is no suitable idle cpu
+	 */
+	thr_worst = worst_running_thread();
+	MPASS(thr_worst != NULL);
+	c = thr_worst->td_oncpu;
+	if (thr_worst->td_priority < td->td_priority)
+		return;
+	if (thr_worst->td_priority > td->td_priority) {
+		thr_worst->td_flags |= TDF_NEEDRESCHED;
+		if ((thr_worst != curthread) && (c != NOCPU))
+			ipi_cpu(c, IPI_AST);
+		return;
+	}
+	/*
+	 * thr_worst->td_priority == td->td_priority
+	 */
+	if (ts->ts_vdeadline < thr_worst->td_sched->ts_vdeadline) {
+		thr_worst->td_flags |= TDF_NEEDRESCHED;
+		if ((thr_worst != curthread) && (c != NOCPU))
+			ipi_cpu(c, IPI_AST);
+	}
+}
+
+void
+sched_rem(struct thread *td)
+{
+	struct td_sched *ts;
+
+	ts = td->td_sched;
+	KASSERT(td->td_flags & TDF_INMEM,
+	    ("sched_rem: thread swapped out"));
+	KASSERT(TD_ON_RUNQ(td),
+	    ("sched_rem: thread not on run queue"));
+	mtx_assert(&sched_lock, MA_OWNED);
+	KTR_STATE2(KTR_SCHED, "thread", sched_tdname(td), "runq rem",
+	    "prio:%d", td->td_priority, KTR_ATTR_LINKED,
+	    sched_tdname(curthread));
+	SDT_PROBE3(sched, , , dequeue, td, td->td_proc, NULL);
+
+	if ((td->td_flags & TDF_NOLOAD) == 0)
+		sched_load_rem();
+	runq_remove(ts->ts_runq, td);
+	TD_SET_CAN_RUN(td);
+}
+
+static struct thread *
+edf_choose(struct rqhead * rqh)
+{
+	struct thread *td;
+	struct thread *td_min;
+	struct td_sched *ts;
+	int deadline_min;
+	int c;
+	
+	td_min = NULL;
+	deadline_min = 0;
+	td = TAILQ_FIRST(rqh);
+	MPASS(td != NULL);
+	if (td != NULL) {
+		td_min = td;
+		deadline_min = td->td_sched->ts_vdeadline;
+	}
+	while (td != NULL) {
+		c = PCPU_GET(cpuid);
+		if (!THREAD_CAN_SCHED(td, c)) {
+			td = TAILQ_NEXT(td, td_runq);
+			continue;
+		}
+		ts = td->td_sched;
+		if (ts->ts_vdeadline < deadline_min) {
+			td_min = td;
+			deadline_min = ts->ts_vdeadline;
+		}
+		td = TAILQ_NEXT(td, td_runq);
+	}
+	return (td_min);
+}
+
+static struct thread *
+runq_choose_bfs(struct runq * rq)
+{
+	struct rqhead *rqh;
+	struct thread *td;
+	struct rqbits * rqb;
+	int pri;
+	int i;
+
+        rqb = &rq->rq_status;
+        for (i = 0; i < RQB_LEN; i++)
+                if (rqb->rqb_bits[i]) {
+                        pri = RQB_FFS(rqb->rqb_bits[i]) + (i << RQB_L2BPW);
+                        CTR3(KTR_RUNQ, "runq_choose_bfs: bits=%#x i=%d pri=%d",
+                            rqb->rqb_bits[i], i, pri);
+			pri = RQB_FFS(rqb->rqb_bits[i]) + (i << RQB_L2BPW);
+			if ((pri == RQ_TIMESHARE) || (pri == RQ_IDLE)) {
+				td = edf_choose(&rq->rq_queues[pri]);
+				return (td);
+			}
+			rqh = &rq->rq_queues[pri];
+			td = TAILQ_FIRST(rqh);
+			KASSERT((td != NULL), ("runq_choose_bfs: no thread on busy queue"));
+			CTR3(KTR_RUNQ,
+			    "runq_choose_bfs: pri=%d thread=%p rqh=%p", pri, td, rqh);
+			return (td);
+                }
+                
+	CTR1(KTR_RUNQ, "runq_choose_bfs: idlethread pri=%d", pri);
+
+	return (NULL);
+}
+
+/*
+ * Select threads to run.  Note that running threads still consume a
+ * slot.
+ */
+struct thread *
+sched_choose(void)
+{
+	struct thread *td;
+	struct runq *rq;
+
+	mtx_assert(&sched_lock,  MA_OWNED);
+
+	rq = &runq;
+	td = runq_choose_bfs(&runq);
+
+	if (td != NULL) {
+		runq_remove(rq, td);
+		td->td_flags |= TDF_DIDRUN;
+
+		KASSERT(td->td_flags & TDF_INMEM,
+		    ("sched_choose: thread swapped out"));
+		return (td);
+	}
+	return (PCPU_GET(idlethread));
+}
+
+void
+sched_preempt(struct thread *td)
+{
+
+	SDT_PROBE2(sched, , , surrender, td, td->td_proc);
+	thread_lock(td);
+	if (td->td_critnest > 1)
+		td->td_owepreempt = 1;
+	else
+		mi_switch(SW_INVOL | SW_PREEMPT | SWT_PREEMPT, NULL);
+	thread_unlock(td);
+}
+
+void
+sched_userret(struct thread *td)
+{
+	/*
+	 * XXX we cheat slightly on the locking here to avoid locking in
+	 * the usual case.  Setting td_priority here is essentially an
+	 * incomplete workaround for not setting it properly elsewhere.
+	 * Now that some interrupt handlers are threads, not setting it
+	 * properly elsewhere can clobber it in the window between setting
+	 * it here and returning to user mode, so don't waste time setting
+	 * it perfectly here.
+	 */
+	KASSERT((td->td_flags & TDF_BORROWING) == 0,
+	    ("thread with borrowed priority returning to userland"));
+	if (td->td_priority != td->td_user_pri) {
+		thread_lock(td);
+		td->td_priority = td->td_user_pri;
+		td->td_base_pri = td->td_user_pri;
+		thread_unlock(td);
+	}
+}
+
+void
+sched_bind(struct thread *td, int cpu)
+{
+	struct td_sched *ts;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED|MA_NOTRECURSED);
+	KASSERT(td == curthread, ("sched_bind: can only bind curthread"));
+
+	ts = td->td_sched;
+
+	td->td_flags |= TDF_BOUND;
+#ifdef SMP
+	ts->ts_runq = &runq; //???
+	if (PCPU_GET(cpuid) == cpu)
+		return;
+
+	mi_switch(SW_VOL, NULL);
+#endif
+}
+
+void
+sched_unbind(struct thread* td)
+{
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	KASSERT(td == curthread, ("sched_unbind: can only bind curthread"));
+	td->td_flags &= ~TDF_BOUND;
+}
+
+int
+sched_is_bound(struct thread *td)
+{
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	return (td->td_flags & TDF_BOUND);
+}
+
+void
+sched_relinquish(struct thread *td)
+{
+	thread_lock(td);
+	mi_switch(SW_VOL | SWT_RELINQUISH, NULL);
+	thread_unlock(td);
+}
+
+int
+sched_load(void)
+{
+	return (sched_tdcnt);
+}
+
+int
+sched_sizeof_proc(void)
+{
+	return (sizeof(struct proc));
+}
+
+int
+sched_sizeof_thread(void)
+{
+	return (sizeof(struct thread) + sizeof(struct td_sched));
+}
+
+fixpt_t
+sched_pctcpu(struct thread *td)
+{
+	struct td_sched *ts;
+	int time_passed;
+	int nticks;
+	fixpt_t pct = 0;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);
+	ts = td->td_sched;
+	
+	switch (td->td_state) {
+	case TDS_RUNNING:
+		if (ts->ts_used < 0) panic("Bad ts_used value\n");
+		nticks = ts->ts_used;
+		break;
+	default:
+		time_passed = ticks - ts->ts_cswtick;
+		nticks = imax(ts->ts_used - time_passed, 0);
+		break;
+	}
+	nticks /= PCT_WINDOW;
+
+	if (nticks > hz) panic("too big nticks value.\n");
+	if (nticks < 0) panic("bad nticks value.\n");
+
+	pct = (FSCALE * ((FSCALE * nticks) / hz)) >> FSHIFT;
+
+	return (pct);
+}
+
+void
+sched_tick(int cnt)
+{
+}
+
+/*
+ * The actual idle process.
+ */
+void
+sched_idletd(void *dummy)
+{
+
+//	for (;;) {
+//		mtx_assert(&Giant, MA_NOTOWNED);
+
+//		while (sched_runnable() == 0)
+//			cpu_idle(0);
+
+//		mtx_lock_spin(&sched_lock);
+//		mi_switch(SW_VOL | SWT_IDLE, NULL);
+//		mtx_unlock_spin(&sched_lock);
+//	}
+        struct pcpuidlestat *stat;
+
+        THREAD_NO_SLEEPING();
+        stat = DPCPU_PTR(idlestat);
+        for (;;) {
+                mtx_assert(&Giant, MA_NOTOWNED);
+
+                while (sched_runnable() == 0) {
+                        cpu_idle(stat->idlecalls + stat->oldidlecalls > 64);
+                        stat->idlecalls++;
+                }
+
+                mtx_lock_spin(&sched_lock);
+                mi_switch(SW_VOL | SWT_IDLE, NULL);
+                mtx_unlock_spin(&sched_lock);
+        }
+}
+
+/*
+ * A CPU is entering for the first time or a thread is exiting.
+ */
+void
+sched_throw(struct thread *td)
+{
+	/*
+	 * Correct spinlock nesting.  The idle thread context that we are
+	 * borrowing was created so that it would start out with a single
+	 * spin lock (sched_lock) held in fork_trampoline().  Since we've
+	 * explicitly acquired locks in this function, the nesting count
+	 * is now 2 rather than 1.  Since we are nested, calling
+	 * spinlock_exit() will simply adjust the counts without allowing
+	 * spin lock using code to interrupt us.
+	 */
+	if (td == NULL) {
+		mtx_lock_spin(&sched_lock);
+		spinlock_exit();
+		PCPU_SET(switchtime, cpu_ticks());
+		PCPU_SET(switchticks, ticks);
+	} else {
+		lock_profile_release_lock(&sched_lock.lock_object);
+		MPASS(td->td_lock == &sched_lock);
+	}
+	mtx_assert(&sched_lock, MA_OWNED);
+	KASSERT(curthread->td_md.md_spinlock_count == 1, ("invalid count"));
+	cpu_throw(td, choosethread());  /* doesn't return */
+}
+
+void
+sched_fork_exit(struct thread *td)
+{
+
+	/*
+	 * Finish setting up thread glue so that it begins execution in a
+	 * non-nested critical section with sched_lock held but not recursed.
+	 */
+	td->td_oncpu = PCPU_GET(cpuid);
+	sched_lock.mtx_lock = (uintptr_t)td;
+	lock_profile_obtain_lock_success(&sched_lock.lock_object,
+	    0, 0, __FILE__, __LINE__);
+	THREAD_LOCK_ASSERT(td, MA_OWNED | MA_NOTRECURSED);
+}
+
+char *
+sched_tdname(struct thread *td)
+{
+#ifdef KTR
+	struct td_sched *ts;
+
+	ts = td->td_sched;
+	if (ts->ts_name[0] == '\0')
+		snprintf(ts->ts_name, sizeof(ts->ts_name),
+		    "%s tid %d", td->td_name, td->td_tid);
+	return (ts->ts_name);
+#else   
+	return (td->td_name);
+#endif
+}
+
+void
+sched_affinity(struct thread *td)
+{
+#ifdef SMP
+	struct td_sched *ts;
+	int cpu;
+
+	THREAD_LOCK_ASSERT(td, MA_OWNED);	
+
+	/*
+	 * Set the TSF_AFFINITY flag if there is at least one CPU this
+	 * thread can't run on.
+	 */
+	ts = td->td_sched;
+	ts->ts_flags &= ~TSF_AFFINITY;
+	CPU_FOREACH(cpu) {
+		if (!THREAD_CAN_SCHED(td, cpu)) {
+			ts->ts_flags |= TSF_AFFINITY;
+			break;
+		}
+	}
+
+	/*
+	 * If this thread can run on all CPUs, nothing else to do.
+	 */
+	if (!(ts->ts_flags & TSF_AFFINITY))
+		return;
+
+	/* Pinned threads and bound threads should be left alone. */
+	if (td->td_pinned != 0 || td->td_flags & TDF_BOUND)
+		return;
+
+        if (!TD_IS_RUNNING(td))
+                return;
+
+	/*
+	 * See if our current CPU is in the set.  If not, force a
+	 * context switch.
+	 */
+	if (THREAD_CAN_SCHED(td, td->td_oncpu))
+		return;
+
+        /*
+         * Force a switch before returning to userspace.  If the
+         * target thread is not running locally send an ipi to force
+         * the issue.
+         */
+        td->td_flags |= TDF_NEEDRESCHED;
+        if (td != curthread)
+                ipi_cpu(cpu, IPI_AST);
+#endif
+}
