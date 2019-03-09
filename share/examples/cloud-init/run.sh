rm -f seed.iso
FILES=$( find /usr/jails/jails-system/ubuntusrv1/cloud-init/ -type f |xargs )

#echo $FILES
#genisoimage -output seed.iso -volid cidata -joliet -rock user-data meta-data
genisoimage -output seed.iso -volid cidata -joliet -rock ${FILES}
#/usr/sbin/makefs -t cd9660 -o label=cidata -o publisher="CBSD" seed.iso /usr/jails/jails-system/ubuntu1/cloud-init


