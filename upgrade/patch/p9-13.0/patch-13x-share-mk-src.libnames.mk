--- src.libnames.mk.orig	2019-02-21 09:19:09.639372000 +0000
+++ src.libnames.mk	2019-02-24 10:42:05.930835000 +0000
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
