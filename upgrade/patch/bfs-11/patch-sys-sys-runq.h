--- runq.h.orig	2014-09-14 13:39:07.000000000 +0400
+++ runq.h	2014-09-14 13:31:13.000000000 +0400
@@ -40,6 +40,13 @@
 #define	RQ_NQS		(64)		/* Number of run queues. */
 #define	RQ_PPQ		(4)		/* Priorities per queue. */
 
+#ifdef SCHED_FBFS
+#define	RQ_IDLE		(RQ_NQS - 1)
+#define	RQ_TIMESHARE	(RQ_IDLE - 1)
+#define	RQ_MIN_REALTIME	(PRI_MIN_REALTIME / 4)
+#define	RQ_MAX_REALTIME	(RQ_TIMESHARE - 1)
+#endif
+
 /*
  * Head of run queues.
  */
