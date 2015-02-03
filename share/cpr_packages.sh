#!/bin/sh
#fix error:"ln: /usr/local/bin/autoconf: File exists"
export LN='/bin/ln -f'
export PKG_SUFX=txz
export PACKAGES=/packages
export DISABLE_VULNERABILITIES=yes

export PATH="/usr/lib/distcc/bin:$PATH"
#export CCACHE_PREFIX="/usr/local/bin/distcc"
export CCACHE_PATH="/usr/bin:/usr/local/bin"
export PATH="/usr/local/libexec/ccache:$PATH:/usr/local/bin:/usr/local/sbin"
#export LC_ALL=en_US.UTF-8

# instead of less -e in portmaster defaults
export PAGER=cat

LOGFILE="/tmp/packages.log"
BUILDLOG="/tmp/build.log"

# fatal error for interactive session.
err()
{
	exitval=$1
	shift
	echo "$*" 1>&2
	echo "$*" >> ${LOGFILE}
	exit $exitval
}

truncate -s0 ${LOGFILE} ${BUILDLOG} 
rm -f /tmp/port_log* > /dev/null 2>&1 ||true
#determine how we have free ccachefs 
#CCACHE_SIZE=`df -m /root/.ccache | tail -n1 |/usr/bin/awk '{print $2}'`
#[ -z "${CCACHE_SIZE}" ] && CCACHE_SIZE="4096"
#/usr/local/bin/ccache -M ${CCACHE_SIZE}m >>${LOGFILE} 2>&1|| err 1 "Cannot set ccache size"

find /tmp/usr/ports -type d -name work -exec rm -rf {} \; || true

PORT_DIRS=$( /bin/cat /tmp/ports_list.txt )

mkdir -p ${PACKAGES}/All >>${LOGFILE} 2>&1 || err 1 "Cannot create PACKAGES/All directory!"

ALLPORTS=$( wc -l /tmp/ports_list.txt |/usr/bin/awk '{printf $1"\n"'} )
PROGRESS=0
PASS=0

#set +o errexit
# config recursive while 
for dir in $PORT_DIRS; do
	PROGRESS=$((PROGRESS + 1))
	pkg info -e $( make -C ${dir} -V PKGNAME) && continue
	#this is hack for determine that we have no options anymore - script dup stdout then we can grep for Dialog-Ascii-specific symbol
#	NOCONF=0
#	while [ $NOCONF -eq 0 ]; do
		echo -e "\033[40;35m Do config-recursive while not set for all options: ${PROGRESS}/${ALLPORTS} \033[0m"
		# script -q /tmp/test.$$ 
		make config-recursive -C ${dir}
		PASS=$(( PASS + 1 ))
		[ ${PASS} -gt ${ALLPORTS} ] && NOCONF=1
		# || break
#		grep "\[" /tmp/test.$$
#		[ $? -eq 1 ] && NOCONF=1
#	done
done
rm -f /tmp/test.$$
# reject any potential dialog popup from misc. broken for save options ports for build stage
echo "BATCH=yes" >> /etc/make.conf

PROGRESS=${ALLPORTS}
#set -o errexit
for dir in $PORT_DIRS; do
	PROGRESS=$((PROGRESS - 1))
	echo -e "\033[40;35m Working on ${dir}. ${PROGRESS} ports left. \033[0m"
	# skip if ports already registered

	if [ ! -d "${dir}" ]; then
		echo -e "\033[40;35m Warning: skip port, no such directory: \033[0;32m${dir} \033[0m"
		continue
	fi

	if [ -f /tmp/buildcontinue ]; then
		cd /tmp/packages
		PORTNAME=$( make -C ${dir} -V PKGNAME )
		pkg info -e ${PORTNAME} >/dev/null 2>&1 || {
			[ -f "./${PORTNAME}.txz" ] && env ASSUME_ALWAYS_YES=yes pkg add ./${PORTNAME}.txz && echo -e "\033[40;35m ${PORTNAME} found and added from cache. \033[0m"
		}
	fi

	pkg info -e $( make -C ${dir} -V PKGNAME ) && continue

	#    yes |portmaster -CK --no-confirm -y -H ${dir} 2>&1|tee >>${BUILDLOG}
#	/usr/local/sbin/portmaster -CK --no-confirm --no-term-title --no-index-fetch -y -B -D ${dir} |tee >>${BUILDLOG}
	/usr/local/sbin/portmaster -CK --no-confirm --no-term-title --no-index-fetch -y -B -D ${dir}

	# clean dir while https://github.com/freebsd/portmaster/pull/18 is not commited
	rm -rf /tmp/usr
done

