#!/bin/sh

SSHUSERNAME="ssh"
SSHPASSWDFILE="/tmp/zfsguru_newsshpasswd.dat"
SSHNEWPASSWD=`cat ${SSHPASSWDFILE}`

# sanity check on password file
if [ ${SSHNEWPASSWD} = "" ]
then
 echo "Could not read password"
 exit 1
fi

# add ssh user if nonexistent
/usr/sbin/pw useradd ${SSHUSERNAME} -K wheel > /dev/null 2>&1

# create home directory
mkdir -p /home/${SSHUSERNAME}
chown ${SSHUSERNAME}:${SSHUSERNAME} /home/${SSHUSERNAME}

# reset password of SSH user
/bin/echo "${SSHNEWPASSWD}" | pw usermod ${SSHUSERNAME} -h 0

# remove temporary password file
/bin/rm -f ${SSHPASSWDFILE}

exit 0
