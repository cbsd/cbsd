--- block_if.c.orig	2019-07-14 14:58:42.330635000 +0300
+++ block_if.c	2019-07-14 15:00:41.384734000 +0300
@@ -25,11 +25,11 @@
  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  * SUCH DAMAGE.
  *
- * $FreeBSD: head/usr.sbin/bhyve/block_if.c 347033 2019-05-02 22:46:37Z jhb $
+ * $FreeBSD$
  */
 
 #include <sys/cdefs.h>
-__FBSDID("$FreeBSD: head/usr.sbin/bhyve/block_if.c 347033 2019-05-02 22:46:37Z jhb $");
+__FBSDID("$FreeBSD$");
 
 #include <sys/param.h>
 #ifndef WITHOUT_CAPSICUM
@@ -42,9 +42,6 @@
 #include <sys/disk.h>
 
 #include <assert.h>
-#ifndef WITHOUT_CAPSICUM
-#include <capsicum_helpers.h>
-#endif
 #include <err.h>
 #include <fcntl.h>
 #include <stdio.h>
@@ -74,6 +71,21 @@
 	BOP_DELETE
 };
 
+#define QOS_TICK_FREQ		10      /* ticks per second */
+#define QOS_MIN_DELAY_US	5000    /* 5ms */
+
+enum qos_type {
+	QOS_MBPS_LIMIT,		/* MB per second */
+	QOS_IOPS_LIMIT,		/* IO per second */
+	QOS_LIMIT_TYPES,	/* should be last  */
+};
+
+struct qos_limit {
+	long long	base_limit;
+	long long	left_quota;
+	void	(*charge)(struct qos_limit *limit, struct blockif_req *br);
+};
+
 enum blockstat {
 	BST_FREE,
 	BST_BLOCK,
@@ -107,6 +119,11 @@
 	pthread_mutex_t		bc_mtx;
 	pthread_cond_t		bc_cond;
 
+	/* access under bc_mtx mutex held */
+	struct qos_limit	bc_limits[QOS_LIMIT_TYPES];
+	int			bc_timeslice;
+	pthread_cond_t		bc_cond_quota;
+
 	/* Request elements and free/pending/busy queues */
 	TAILQ_HEAD(, blockif_elem) bc_freeq;       
 	TAILQ_HEAD(, blockif_elem) bc_pendq;
@@ -125,6 +142,110 @@
 
 static struct blockif_sig_elem *blockif_bse_head;
 
+static void
+quota_mbps_charge(struct qos_limit *limit, struct blockif_req *br)
+{
+	limit->left_quota -= br->br_resid;
+}
+
+static void
+quota_iops_charge(struct qos_limit *limit, struct blockif_req *br)
+{
+	limit->left_quota--;
+}
+
+static bool
+blockif_qos_is_off(struct blockif_ctxt *bc)
+{
+	for (int i = 0; i < QOS_LIMIT_TYPES; ++i) {
+		if (bc->bc_limits[i].base_limit != 0)
+			return false;
+	}
+	return true;
+}
+
+static void
+blockif_quota_update(struct blockif_ctxt *bc)
+{
+	for (int i = 0; i < QOS_LIMIT_TYPES; ++i) {
+		struct qos_limit *lp = &bc->bc_limits[i];
+
+		if (lp->base_limit != 0)
+			lp->left_quota = lp->base_limit;
+	}
+}
+
+static bool
+blockif_quota_exceeded(struct blockif_ctxt *bc)
+{
+	for (int i = 0; i < QOS_LIMIT_TYPES; ++i) {
+		if (bc->bc_limits[i].left_quota < 0)
+			return true;
+	}
+	return false;
+}
+
+static void
+blockif_quota_charge(struct blockif_ctxt *bc, struct blockif_req *br)
+{
+	for (int i = 0; i < QOS_LIMIT_TYPES; ++i) {
+		struct qos_limit *lp = &bc->bc_limits[i];
+
+		if (lp->charge)
+			lp->charge(lp, br);
+	}
+}
+
+/* should be called with held mtx */
+static void
+blockif_qos(struct blockif_ctxt *bc, struct blockif_elem *be)
+{
+	struct blockif_req *br = be->be_req;
+
+	if (blockif_qos_is_off(bc) ||
+		(be->be_op != BOP_READ && be->be_op != BOP_WRITE))
+		return;
+
+	while (blockif_quota_exceeded(bc) && !bc->bc_closing) {
+		/* another worker has already reached limit */
+		pthread_cond_wait(&bc->bc_cond_quota, &bc->bc_mtx);
+	}
+
+	blockif_quota_charge(bc, br);
+
+	if (blockif_quota_exceeded(bc)) {
+		while (blockif_quota_exceeded(bc) && !bc->bc_closing) {
+			struct timespec ts = {};
+			int tid, err;
+
+			err = clock_gettime(CLOCK_MONOTONIC_PRECISE, &ts);
+			assert(err == 0);
+
+			/* get current tick */
+			tid = ((time_t)(short)ts.tv_sec * QOS_TICK_FREQ) +
+				((unsigned long long)ts.tv_nsec * QOS_TICK_FREQ)/1000000000;
+
+			if (bc->bc_timeslice == tid) {
+				int delay = ts.tv_nsec/1000;	/* usec */
+
+				/* Use extra 1ms to cross over tick boundary */
+				delay = 1000000/QOS_TICK_FREQ - delay % (1000000/QOS_TICK_FREQ) + 1000;
+				delay = MAX(delay, QOS_MIN_DELAY_US);
+
+				pthread_mutex_unlock(&bc->bc_mtx);
+				usleep(delay);
+				pthread_mutex_lock(&bc->bc_mtx);
+				tid++;
+			}
+
+			/* new period, update quota and continue */
+			blockif_quota_update(bc);
+			bc->bc_timeslice = tid;
+		}
+		pthread_cond_broadcast(&bc->bc_cond_quota);
+	}
+}
+
 static int
 blockif_enqueue(struct blockif_ctxt *bc, struct blockif_req *breq,
 		enum blockop op)
@@ -347,6 +468,7 @@
 	pthread_mutex_lock(&bc->bc_mtx);
 	for (;;) {
 		while (blockif_dequeue(bc, t, &be)) {
+			blockif_qos(bc, be);
 			pthread_mutex_unlock(&bc->bc_mtx);
 			blockif_proc(bc, be, buf);
 			pthread_mutex_lock(&bc->bc_mtx);
@@ -409,6 +531,8 @@
 	off_t size, psectsz, psectoff;
 	int extra, fd, i, sectsz;
 	int nocache, sync, ro, candelete, geom, ssopt, pssopt;
+	struct qos_limit qos_limits[QOS_LIMIT_TYPES];
+	int quota;
 #ifndef WITHOUT_CAPSICUM
 	cap_rights_t rights;
 	cap_ioctl_t cmds[] = { DIOCGFLUSH, DIOCGDELETE };
@@ -421,6 +545,7 @@
 	nocache = 0;
 	sync = 0;
 	ro = 0;
+	memset(&qos_limits, 0, sizeof (qos_limits));
 
 	/*
 	 * The first element in the optstring is always a pathname.
@@ -441,7 +566,17 @@
 			;
 		else if (sscanf(cp, "sectorsize=%d", &ssopt) == 1)
 			pssopt = ssopt;
-		else {
+		else if (sscanf(cp, "mbps_limit=%d", &quota) == 1) {
+			long long q = quota;
+
+			q = (q << 20) / QOS_TICK_FREQ;
+			qos_limits[QOS_MBPS_LIMIT].base_limit = q;
+			qos_limits[QOS_MBPS_LIMIT].charge = quota_mbps_charge;
+		} else if (sscanf(cp, "iops_limit=%d", &quota) == 1) {
+			quota = quota / QOS_TICK_FREQ;
+			qos_limits[QOS_IOPS_LIMIT].base_limit = MAX(quota, 1);
+			qos_limits[QOS_IOPS_LIMIT].charge = quota_iops_charge;
+		} else {
 			fprintf(stderr, "Invalid device option \"%s\"\n", cp);
 			goto err;
 		}
@@ -476,7 +611,7 @@
 	if (ro)
 		cap_rights_clear(&rights, CAP_FSYNC, CAP_WRITE);
 
-	if (caph_rights_limit(fd, &rights) == -1)
+	if (cap_rights_limit(fd, &rights) == -1 && errno != ENOSYS)
 		errx(EX_OSERR, "Unable to apply rights for sandbox");
 #endif
 
@@ -507,7 +642,7 @@
 		psectsz = sbuf.st_blksize;
 
 #ifndef WITHOUT_CAPSICUM
-	if (caph_ioctls_limit(fd, cmds, nitems(cmds)) == -1)
+	if (cap_ioctls_limit(fd, cmds, nitems(cmds)) == -1 && errno != ENOSYS)
 		errx(EX_OSERR, "Unable to apply rights for sandbox");
 #endif
 
@@ -556,8 +691,11 @@
 	bc->bc_sectsz = sectsz;
 	bc->bc_psectsz = psectsz;
 	bc->bc_psectoff = psectoff;
+	for (int i = 0; i < QOS_LIMIT_TYPES; ++i)
+		bc->bc_limits[i] = qos_limits[i];
 	pthread_mutex_init(&bc->bc_mtx, NULL);
 	pthread_cond_init(&bc->bc_cond, NULL);
+	pthread_cond_init(&bc->bc_cond_quota, NULL);
 	TAILQ_INIT(&bc->bc_freeq);
 	TAILQ_INIT(&bc->bc_pendq);
 	TAILQ_INIT(&bc->bc_busyq);
@@ -576,7 +714,6 @@
 err:
 	if (fd >= 0)
 		close(fd);
-	free(nopt);
 	return (NULL);
 }
 
