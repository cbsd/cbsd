--- src.libnames.mk-orig	2017-10-07 13:23:08.481906000 +0300
+++ src.libnames.mk	2017-10-07 13:22:45.000000000 +0300
@@ -56,6 +56,7 @@
 		${_INTERNALLIBS} \
 		${LOCAL_LIBRARIES} \
 		80211 \
+		9p \
 		alias \
 		archive \
 		asn1 \
@@ -209,6 +210,7 @@
 # Each library's LIBADD needs to be duplicated here for static linkage of
 # 2nd+ order consumers.  Auto-generating this would be better.
 _DP_80211=	sbuf bsdxml
+_DP_9p=		sbuf
 _DP_archive=	z bz2 lzma bsdxml
 _DP_zstd=	pthread
 .if ${MK_BLACKLIST} != "no"
