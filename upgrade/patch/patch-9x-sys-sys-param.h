--- param.h.bak	2013-07-05 16:20:10.809413310 +0400
+++ param.h	2013-07-05 16:20:24.530413061 +0400
@@ -96,7 +96,7 @@
 
 #define	MAXCOMLEN	19		/* max command name remembered */
 #define	MAXINTERP	PATH_MAX	/* max interpreter file name length */
-#define	MAXLOGNAME	17		/* max login name length (incl. NUL) */
+#define	MAXLOGNAME	33		/* max login name length (incl. NUL) */
 #define	MAXUPRC		CHILD_MAX	/* max simultaneous processes */
 #define	NCARGS		ARG_MAX		/* max bytes for an exec function */
 #define	NGROUPS		(NGROUPS_MAX+1)	/* max number groups */
