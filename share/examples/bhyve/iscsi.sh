#!/bin/sh
while getopts "c:d:e:g:hl:r:w:" opt; do
	case "${opt}" in
		c) conf="${OPTARG}" ;;
		d) debug="${OPTARG}" ;;
		e) exit_action="${OPTARG}" ;;
		g) debug_engine="${OPTARG}" ;;
		l) orig_logfile="${OPTARG}" ;;
		r) restore_checkpoint="${OPTARG}" ;;
		w) workdir="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

if [ ! -r ${conf} ]; then
	echo "no conf"
	exit 1
fi

# Update settings tables for tablet column
: ${distdir="/usr/local/cbsd"}
[ ! -r "${distdir}/subr/cbsdbootstrap.subr" ] && exit 1
. ${distdir}/subr/cbsdbootstrap.subr || exit 1

. ${conf}

if [ ! -h ${mydsk} ]; then
	echo "not symlink: $mydsk"
	exit 1
fi

myvol=$( ${READLINK_CMD} ${mydsk} )

if [ -z "${myvol}" ]; then
	echo "no zvol"
	exit 1
fi

# listen_v4 to config
# listen_v6 to config
#listen_v6="listen [::]"
#listen_v4="listen 0.0.0.0"
listen_v6=
listen_v4="listen 172.16.0.1"

### remove old ctl: DEBUG
rm -f /etc/ctl.conf

if [ ! -r /etc/ctl.conf ]; then
	${CAT_CMD} > /etc/ctl.conf <<EOF
portal-group pg0 {						# CBSD_pg0_${jname}
	discovery-auth-group no-authentication			# CBSD_pg0_${jname}
	${listen_v4}						# CBSD_pg0_${jname}
	${listen_v6}						# CBSD_pg0_${jname}
}
EOF
fi

${CP_CMD} -a /etc/ctl.conf /tmp/ctl.conf
${GREP_CMD} -v CBSD_iscsi_${jname} /tmp/ctl.conf > /etc/ctl.conf

## 93.100.25.10 - RemoteIP get/pass  - клиент
## 142.132.155.251 - serverIP

# для HOME/LAN
# 172.16.0.3 - RemoteIP get/pass - клиент
# 172.16.0.1 - serverIP
${CAT_CMD} >> /etc/ctl.conf <<EOF
target iqn.172.16.0.3:target0 {		# CBSD_iscsi_${jname}
	auth-group no-authentication		# CBSD_iscsi_${jname}
	portal-group pg0			# CBSD_iscsi_${jname}
	lun 0 {					# CBSD_iscsi_${jname}
		blocksize 512			# CBSD_iscsi_${jname}
		path ${myvol}			# CBSD_iscsi_${jname}
		option vendor "${jname}"	# CBSD_iscsi_${jname}
	}					# CBSD_iscsi_${jname}
}						# CBSD_iscsi_${jname}
EOF

${CHMOD_CMD} 0400 /etc/ctl.conf
/usr/local/cbsd/misc/cbsdsysrc ctld_enable="YES"
${SERVICE_CMD} ctld restart


cat <<EOF
======== CLIENT SETUP INFO ==========

/usr/local/cbsd/misc/cbsdsysrc iscsid_enable="YES" 
service iscsid restart

iscsictl -A -p 172.16.0.1 -t iqn.172.16.0.3:target0
iscsictl

EOF

