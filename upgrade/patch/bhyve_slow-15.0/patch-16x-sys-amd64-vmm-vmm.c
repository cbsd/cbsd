diff --git a/sys/amd64/vmm/vmm.c b/sys/amd64/vmm/vmm.c
index 050cc93d26..f1b222e92b 100644
--- a/sys/amd64/vmm/vmm.c
+++ b/sys/amd64/vmm/vmm.c
@@ -64,6 +64,8 @@
 #include <x86/apicreg.h>
 #include <x86/ifunc.h>
 
+extern uint64_t	tsc_freq;
+
 #include <machine/vmm.h>
 #include <machine/vmm_instruction_emul.h>
 #include <machine/vmm_snapshot.h>
@@ -181,6 +183,16 @@ static int trap_wbinvd;
 SYSCTL_INT(_hw_vmm, OID_AUTO, trap_wbinvd, CTLFLAG_RDTUN, &trap_wbinvd, 0,
     "WBINVD triggers a VM-exit");
 
+static int vcpu_delay_us;
+SYSCTL_INT(_hw_vmm, OID_AUTO, vcpu_delay_us, CTLFLAG_RWTUN,
+    &vcpu_delay_us, 0,
+    "Base microseconds to busy-wait in host after each VM-exit");
+
+static int vcpu_slowdown_pct;
+SYSCTL_INT(_hw_vmm, OID_AUTO, vcpu_slowdown_pct, CTLFLAG_RWTUN,
+    &vcpu_slowdown_pct, 0,
+    "Extra busy-wait time as a percentage of vcpu runtime per VM-exit");
+
 /* global statistics */
 VMM_STAT(VCPU_MIGRATIONS, "vcpu migration across host cpus");
 VMM_STAT(VMEXIT_COUNT, "total number of vm exits");
@@ -1126,7 +1138,7 @@ vm_run(struct vcpu *vcpu)
 	struct vm_eventinfo evinfo;
 	int error, vcpuid;
 	struct pcb *pcb;
-	uint64_t tscval;
+	uint64_t tscval, runtime;
 	struct vm_exit *vme;
 	bool retu, intr_disabled;
 	pmap_t pmap;
@@ -1163,10 +1175,25 @@ vm_run(struct vcpu *vcpu)
 
 	save_guest_fpustate(vcpu);
 
-	vmm_stat_incr(vcpu, VCPU_TOTAL_RUNTIME, rdtsc() - tscval);
+	runtime = rdtsc() - tscval;
+	vmm_stat_incr(vcpu, VCPU_TOTAL_RUNTIME, runtime);
 
 	critical_exit();
 
+	if (__predict_false(vcpu_delay_us > 0 || vcpu_slowdown_pct > 0)) {
+		uint64_t usec;
+
+		usec = vcpu_delay_us;
+		if (vcpu_slowdown_pct > 0 && tsc_freq != 0) {
+			uint64_t runtime_us;
+
+			runtime_us = runtime * 1000000ULL / tsc_freq;
+			usec += runtime_us * (uint64_t)vcpu_slowdown_pct / 100ULL;
+		}
+		if (usec > 0)
+			DELAY((int)usec);
+	}
+
 	if (error == 0) {
 		retu = false;
 		vcpu->nextrip = vme->rip + vme->inst_length;
