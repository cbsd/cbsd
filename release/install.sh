#!/bin/sh
MINIMALSIZEGB=3

. /usr/local/cbsd/nc.subr
. /usr/local/cbsd/tools.subr

# fatal error. Print message end quit with exitval
err()
{
    exitval=$1
    shift
    echo "$*"
    exit $exitval
}

WELCOME="== Welcome to CBSD Console install"

select_dsk()
{
DSK=`/usr/local/cbsd/tools/disks-list |cut -d : -f1 |xargs`

[ -n "${DSK}" ] || err 1 "Hard Disk not found in you system. Install impossible, back to shell"

answ=
while [ "$answ" != "ok" ]; do
printf "You have disks: [${DSK}]\n"
echo "Which one of them I should use for installation?"
echo "(type disk name or \"exit\" for break)"
read answ leftover

[ "${answ}" != "exit" ] || err 1 "Escape to shell"

mydsk=
for i in ${DSK}; do
if [ "$i" = "${answ}" ]; then
mydsk=$answ
answ="ok"
fi
done

if [ -z "${mydsk}" ]; then
echo "Bad choice. Try again"
fi
done
}


install_ufs()
{
#mydsk="md0"
dsk="/dev/${mydsk}"

KB="`diskinfo -v ${dsk} | grep 'bytes' | cut -d '#' -f 1 | tr -s '\t' ' ' | tr -d ' '`"
GB=$((${KB} / 1024 / 1024 / 1024 ))

[ $GB -ge ${MINIMALSIZEGB} ] || err 1 "Disk to small. You must have > ${MINIMALSIZEGB}GB disk for installing CBSD"

SWPMBSIZE="300"

# dirty cleanup
for i in `jot 10`; do
gpart delete -i $i $dsk > /dev/null 2>&1
done

gpart destroy $dsk > /dev/null 2>&1
###

echo "DEBUG"
gpart create -s gpt $dsk
gpart add -s 64K -t freebsd-boot $dsk
gpart add -s ${SWPMBSIZE}M -t freebsd-swap -l swp0 $dsk
gpart add -t freebsd-ufs -l dsk0 $dsk
gpart bootcode -b /boot/pmbr -p /boot/gptboot -i 1 $dsk

stat ${dsk}p3 > /dev/null 2>&1
[ $? -eq 0 ] || err 1 "Can create gpt partition"

newfs -n ${dsk}p3
mkdir /tmp/mnt
mount ${dsk}p3 /tmp/mnt
/usr/local/bin/rsync -Havz --exclude=/tmp/* --exclude=/rescue / /tmp/mnt

cat > /tmp/mnt/etc/fstab << EOF
${dsk}p3 / ufs rw 1 1
${dsk}p2 none swap sw 0 0
EOF

/usr/sbin/sysrc -qf /tmp/mnt/boot/loader.conf bitmap_load=NO

cd /
umount /tmp/mnt
echo "You can eject install media and reboot system now"
}


select_nic()
{
NIC=`/usr/local/cbsd/misc/nics-list|xargs`

[ -n "${NIC}" ] || err 1 "Network Interface not found in you system. Install impossible, back to shell"

answ=
while [ "$answ" != "ok" ]; do
printf "You have : [${NIC}]\n"
echo "Which one of them i should use for managment/network access?"
echo "(type nic name or \"exit\" for break)"
read answ leftover
[ "${answ}" != "exit" ] || err 1 "Escape to shell"
mynic=
for i in ${NIC}; do
if [ "$i" = "${answ}" ]; then
mynic=$answ
answ="ok"
fi
done

if [ -z "${mynic}" ]; then
echo "Bad choice. Try again"
fi
done
}

config_net()
{
DHCP=0

if getyesno "Should use automatic network configure (DHCP) [yes/no]?"; then
    echo "Try to detect via DHCP..."
    /sbin/dhclient ${mynic}
	if [ `ifconfig -c ${mynic} |grep "inet "` = 0 ]; then
	    echo "No DHCP servers found. please set ip manually"
	else
	    DHCP=1; return
	fi
fi

netok=
while [ "$netok" != "ok" ]; do
    echo "Enter IPv4 address ( x.x.x.x )"
    read myip
    echo "Enter Network Mask ( x.x.x.x )"
    read mymask
    echo "Enter Gateway address ( x.x.x.x )"
    read mygw

if getyesno "IP: ${myip}. MASK: ${mymask}. Gateway: ${mygw}. Correct? [yes/no]"; then
	netok="ok"
fi
done
}

apply_net()
{
if [ $DHCP -eq 0 ]; then
	/usr/sbin/sysrc -qf /etc/rc.conf ifconfig_${mynic}="${myip} netmask ${mymask}"
	/usr/sbin/sysrc -qf /etc/rc.conf defaultrouter=${mygw}
else
	/usr/sbin/sysrc -qf /etc/rc.conf ifconfig_${mynic}=DHCP
	/usr/sbin/sysrc -qf /etc/rc.conf defaultrouter=NO
fi
}


make_rcconf()
{
#cat > /etc/rc.conf <<EOF
#hostname="nodeXX.my.domain"
#syslogd_flags="-ss -c"
#EOF
}

config_dns()
{
#IF DHCP then configure dhclient prepend for 127.0.0.1
#
}


configure_node()
{


}


end_config()
{
# restore original files
[ ! -f /etc/ttys-orig ] || mv /etc/ttys-orig /etc/ttys
[ ! -f /root/.cshrc-orig ] || mv /root/.cshrc-orig /root/.cshrc
}

check_hw()
{
NIC=`/usr/local/cbsd/misc/nics-list|xargs`
[ -n "${NIC}" ] || err 1 "Network Interface not found in you system. Install impossible, back to shell"

}


webface()
{
select_nic
config_net

if [ $DHCP -eq 0 ]; then
echo ifconfig $mynic $myip netmask $mymask
echo route add default $mygw
fi



}





### MAIN ###
vidcontrol 80x30 >/dev/null 2>&1
check_hw

# Select install mode
if getyesno "Do you want configure CBSD via WEB interface?"; then
    echo "Sorry. Not implemented yes ;-)"
#    webface
#    exit
fi

# Determine my position and choose correct direction
if [ `kenv vfs.root.mountfrom |grep -c CBSD_INSTALL` = 1 ]; then
printf "$WELCOME: phase1 - install to disk ==\n"
select_dsk
install_ufs
exit
fi

printf "$WELCOME: phase2 - setup you CBSD system ==\n"
make_rcconf
select_nic
config_net
apply_net
config_dns
end_config
printf "$WELCOME: phase3 - cbsd initenv ==\n"

answ=
while [ "$answ" != "ok" ]; do
printf "Enter CBSD work dir (All data will be is here)\n"
printf "(type path, e.g: /usr/jails or exit for break)\n"

read answ leftover
[ "${answ}" != "exit" ] || err 1 "Escape to shell"

echo "env workdir=\"$answ\" /usr/local/cbsd/tools/initenv" > /tmp/tmpworkdir

if getyesno "Install to ${answ}, sure?"; then
    answ="ok"
fi
done


sh /tmp/tmpworkdir && rm -f /tmp/tmpworkdir
