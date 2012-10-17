# $FreeBSD: src/etc/csh.cshrc,v 1.3.62.1 2011/09/23 00:51:37 kensmith Exp $
#
# System-wide .cshrc file for csh(1).
# mc
setenv DISTCC_HOSTS 'localhost 78.46.220.81,lzo 88.198.136.97,lzo'
setenv CCACHE_PREFIX /usr/local/bin/distcc
setenv CCACHE_PATH /usr/bin:/usr/local/bin
setenv CCACHE_DIR "/root/.ccache"
