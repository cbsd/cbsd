#!/bin/sh
# external mounter script sample, e.g /root/ntfs-3g.sh
# fstab line:
# /root/ntfs-3g.sh        /home/web/downloads external rw 0 0
#
usage()
{
	printf "[jail] external mounter script sample\n"
	printf " -j jname (optional)n"
	printf " -o options\n"
	printf " -p full (hoster) path for mountpoint in jail\n"
	exit 0
}

[ -z "${1}" -o "${1}" = "--help" ] && usage

# MAIN
while getopts "j:o:p:" opt; do
	case "${opt}" in
		j) jname="${OPTARG}" ;;
		o) options="${OPTARG}" ;;
		p) path="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

/usr/local/bin/ntfs-3g -o ${options} /dev/da0p1 ${path}
exit $?
