--- src.libnames.mk.orig	2018-11-13 23:17:44.750411000 +0300
+++ src.libnames.mk	2018-11-30 15:14:56.457248000 +0300
@@ -57,6 +57,7 @@
 		${_INTERNALLIBS} \
 		${LOCAL_LIBRARIES} \
 		80211 \
+		9p \
 		alias \
 		archive \
 		asn1 \
@@ -213,6 +214,7 @@
 # Each library's LIBADD needs to be duplicated here for static linkage of
 # 2nd+ order consumers.  Auto-generating this would be better.
 _DP_80211=	sbuf bsdxml
+_DP_9p=		sbuf
 _DP_archive=	z bz2 lzma bsdxml
 _DP_zstd=	pthread
 .if ${MK_BLACKLIST} != "no"
