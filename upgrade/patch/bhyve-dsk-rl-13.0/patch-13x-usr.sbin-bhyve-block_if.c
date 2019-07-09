--- block_if.c.orig	2019-07-09 18:02:25.220818000 +0300
+++ block_if.c	2019-07-09 18:12:45.719183000 +0300
@@ -74,6 +74,14 @@
 	BOP_DELETE
 };
 
+#define QOS_TICK_FREQ          10      /* ticks per second */
+#define QOS_MIN_DELAY_US       10000   /* usec */
+
+enum qos_type {
+	QOS_MPS = 1,    /* MB per second */
+	QOS_IOPS,       /* IO per second */
+};
+
 enum blockstat {
 	BST_FREE,
 	BST_BLOCK,
@@ -107,6 +115,13 @@
 	pthread_mutex_t		bc_mtx;
 	pthread_cond_t		bc_cond;
 
+	/* access under bc_mtx mutex held */
+	int			bc_qos_type;
+	int			bc_qos_limit;
+	int			bc_timeslice;
+	int			bc_timeslice_quota;
+	pthread_cond_t		bc_cond_quota;
+
 	/* Request elements and free/pending/busy queues */
 	TAILQ_HEAD(, blockif_elem) bc_freeq;       
 	TAILQ_HEAD(, blockif_elem) bc_pendq;
@@ -125,6 +140,55 @@
 
 static struct blockif_sig_elem *blockif_bse_head;
 
+/* should be called with held mtx */
+static void
+blockif_qos(struct blockif_ctxt *bc, struct blockif_elem *be)
+{
+	struct blockif_req *br = be->be_req;
+
+	if (bc->bc_qos_limit == 0 ||
+		(be->be_op != BOP_READ && be->be_op != BOP_WRITE))
+		return;
+
+	while (bc->bc_timeslice_quota < 0 && !bc->bc_closing) {
+		/* another worker has already reached limit */
+		pthread_cond_wait(&bc->bc_cond_quota, &bc->bc_mtx);
+	}
+
+	bc->bc_timeslice_quota -= (bc->bc_qos_type == QOS_MPS) ? br->br_resid : 1;
+
+	if (bc->bc_timeslice_quota < 0) {
+		while (bc->bc_timeslice_quota < 0 && !bc->bc_closing) {
+			struct timespec ts = {};
+			int tid, err;
+
+			err = clock_gettime(CLOCK_MONOTONIC_PRECISE, &ts);
+			assert(err == 0);
+
+			/* get tick id */
+			tid = ((time_t)(short)ts.tv_sec * QOS_TICK_FREQ) +
+				((unsigned long long)ts.tv_nsec * QOS_TICK_FREQ)/1000000000;
+
+			if (bc->bc_timeslice == tid) {
+				int delay = ts.tv_nsec/1000;    /* usec */
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
+			/* new period, update quota and continue */
+			bc->bc_timeslice_quota += bc->bc_qos_limit;
+			bc->bc_timeslice = tid;
+		}
+		pthread_cond_broadcast(&bc->bc_cond_quota);
+	}
+}
+
 static int
 blockif_enqueue(struct blockif_ctxt *bc, struct blockif_req *breq,
 		enum blockop op)
@@ -347,6 +411,7 @@
 	pthread_mutex_lock(&bc->bc_mtx);
 	for (;;) {
 		while (blockif_dequeue(bc, t, &be)) {
+			blockif_qos(bc, be);
 			pthread_mutex_unlock(&bc->bc_mtx);
 			blockif_proc(bc, be, buf);
 			pthread_mutex_lock(&bc->bc_mtx);
@@ -409,6 +474,8 @@
 	off_t size, psectsz, psectoff;
 	int extra, fd, i, sectsz;
 	int nocache, sync, ro, candelete, geom, ssopt, pssopt;
+	int qos_type;
+	int qos_limit;
 #ifndef WITHOUT_CAPSICUM
 	cap_rights_t rights;
 	cap_ioctl_t cmds[] = { DIOCGFLUSH, DIOCGDELETE };
@@ -421,6 +488,8 @@
 	nocache = 0;
 	sync = 0;
 	ro = 0;
+	qos_type = 0;
+	qos_limit = 0;
 
 	/*
 	 * The first element in the optstring is always a pathname.
@@ -441,7 +510,12 @@
 			;
 		else if (sscanf(cp, "sectorsize=%d", &ssopt) == 1)
 			pssopt = ssopt;
-		else {
+		else if (sscanf(cp, "mbps_limit=%d", &qos_limit) == 1) {
+			qos_type = QOS_MPS;
+			qos_limit <<= 20;
+		} else if (sscanf(cp, "iops_limit=%d", &qos_limit) == 1) {
+			qos_type = QOS_IOPS;
+		} else {
 			fprintf(stderr, "Invalid device option \"%s\"\n", cp);
 			goto err;
 		}
@@ -556,8 +630,12 @@
 	bc->bc_sectsz = sectsz;
 	bc->bc_psectsz = psectsz;
 	bc->bc_psectoff = psectoff;
+	bc->bc_qos_limit = qos_limit / QOS_TICK_FREQ;
+	bc->bc_timeslice_quota = bc->bc_qos_limit;
+	bc->bc_qos_type = qos_type;
 	pthread_mutex_init(&bc->bc_mtx, NULL);
 	pthread_cond_init(&bc->bc_cond, NULL);
+	pthread_cond_init(&bc->bc_cond_quota, NULL);
 	TAILQ_INIT(&bc->bc_freeq);
 	TAILQ_INIT(&bc->bc_pendq);
 	TAILQ_INIT(&bc->bc_busyq);
