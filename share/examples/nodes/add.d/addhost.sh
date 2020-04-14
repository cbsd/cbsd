#!/bin/sh
# CBSD Project, custom script for add.d node hook sample

# MAIN()
# getopt: -n ${nodename} -i ${ip} -p ${port} -k ${keyfile}
while getopts "n:i:p:k:" opt; do
	case "${opt}" in
		n) nodename="${OPTARG}" ;;
		i) ip="${OPTARG}" ;;
		p) port="${OPTARG}" ;;
		k) keyfile="${OPTARG}" ;;
	esac
	shift $(( ${OPTIND} - 1 ))
done

date > /tmp/testfile.log
echo "node added: nodename:${nodename} ip:${ip} port:${port} keyfile:${keyfile}" >> /tmp/testfile.log

exit 0
