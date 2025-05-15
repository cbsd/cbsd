# Managing multiple Jails using Shell Scripts

Between manual control of each Jail using **cbsd jlogin** and the setting up of centralized orchestration systems
like **Puppet** or **Ansible** stay a method based on scripts in the language Shell.

Using framework **cbsd** and command like **jset/jget/jconfig/jexec** you can implement quite complex
scenarios for the configuration and management of multiple cells.

Below are examples of writing such scripts to solve all sorts of group tasks inside Jails.

# We test the presence of vulnerable software in the Jails.

The simplest script that runs in each jail is a command **pkg audit -F** \- checking for installed software
with known vulnerabilities in the FreeBSD project.

```
#!/bin/sh

echo «Checking local SYSTEM»
pkg audit -F
echo ""
echo «Checking DokuWiki JAIL»
/usr/local/bin/cbsd jexec jname=dokuwiki pkg audit -F
echo ""
echo «Checking OwnCloud JAIL»
/usr/local/bin/cbsd jexec jname=owncloud pkg audit -F
echo ""
echo «Checking FTP Backup JAIL»
/usr/local/bin/cbsd jexec jname=ftpbackup pkg audit -F
echo ""
echo «Checking SAMBA JAIL»
/usr/local/bin/cbsd jexec jname=samba pkg audit –F

```

Example of script output after working off:

```
Checking local SYSTEM
vulnxml file up-to-date
0 problem(s) in the installed packages found.

Checking DokuWiki JAIL
vulnxml file up-to-date
0 problem(s) in the installed packages found.

Checking OwnCloud JAIL
vulnxml file up-to-date
0 problem(s) in the installed packages found.

Checking FTP Backup JAIL
vulnxml file up-to-date
0 problem(s) in the installed packages found.

Checking SAMBA JAIL
vulnxml file up-to-date
0 problem(s) in the installed packages found.

```

A more compact version of the script that uses the loop and status check Jail.

```
#!/bin/sh

for jail in $(/usr/local/bin/cbsd jorder); do

        jstatus1=`/usr/local/bin/cbsd jstatus ${jail}`

        if [ $jstatus1 -ne "0" ]; then
                /usr/local/bin/cbsd jexec jname=${jail} pkg audit -F
        fi

done

```

# Updating the database of known packages and displaying information about updates

Similar to the previous idea - we perform **pkg upgrade** with key **-n** inside of each jail. Result - update the list of available jails
packages in **pkg** from the repositories and display available updates.

```
#!/bin/sh

echo «Checking local SYSTEM»
pkg upgrade -n
echo ""
echo «Checking DokuWiki JAIL»
/usr/local/bin/cbsd jexec jname=dokuwiki pkg upgrade -n
echo ""
echo «Checking OwnCloud JAIL»
/usr/local/bin/cbsd jexec jname=owncloud pkg upgrade -n
echo ""
echo «Checking FTP Backup JAIL»
/usr/local/bin/cbsd jexec jname=ftpbackup pkg upgrade -n
echo ""
echo «Checking SAMBA JAIL»
/usr/local/bin/cbsd jexec jname=samba pkg upgrade –n

```

# Obtaining instantaneous values of jail memory consumption

The task is simple - we want to know how many running jails use RAM.

```
#!/bin/sh

cbsd jrctl mode=show | egrep -i "\-\-\-|memoryuse" | grep -v "\=0"

```

The result of the script execution

```
--- dokuwiki ---
memoryuse=31M
vmemoryuse=645M
--- owncloud ---
memoryuse=46M
vmemoryuse=1948M
--- samba ---
memoryuse=16M
vmemoryuse=1680M
--- ftpbackup ---
memoryuse=1172K
vmemoryuse=42M
--- asterisk ---
memoryuse=25M
vmemoryuse=889M

```

# Run several scripts and send them by mail

So - we have several useful scripts that get the information we need about the cells.
It is necessary to solve the task of periodically starting them and getting output to e-mail.

```
#!/bin/sh

sleep 1
echo "To: vershinin.e@gmail.com" > /root/Scripts/audit-pkg.mail
echo "Subject: Audit PKG on MAIN and JAILed systems!!!" >> /root/Scripts/audit-pkg.mail
echo "" >> /root/Scripts/audit-pkg.mail
echo "" >> /root/Scripts/audit-pkg.mail
sleep 1
`/root/Scripts/pkg-audit-all-sys.sh >> /root/Scripts/audit-pkg.mail`
sleep 1
`/root/Scripts/pkg-upgrade-all-sys.sh >> /root/Scripts/audit-pkg.mail`
sleep 1
`/root/Scripts/memory-use-jails.sh >> /root/Scripts/audit-pkg.mail`
sleep 1
`cat /root/Scripts/audit-pkg.mail | /usr/local/bin/msmtp vershinin.e@gmail.com`
sleep 1

```

Mail client settings msmtp in file /root/.msmtprc

```
account default
host smtp.gmail.com
maildomain gmail.com
auto_from on
port 587
from vershinin.asterisk@gmail.com
tls on
tls_certcheck off

auth on
user vershinin.asterisk
password XXXXXXXXXXXX
logfile /var/log/msmtp.log

# Syslog logging with facility LOG_MAIL instead of the default LOG_USER.
syslog LOG_MAIL

```

# Regular cell backup

A script for the regular unloading of jails and their saving in a certain place with a specific name.

```
#!/bin/sh

jailname=$1

CBSDPATH=/CBSD

JAILBACKUPTARGET=/data/JAILS

backupdate=`/bin/date "+%Y-%m-%d"`

jstatus=`/usr/local/bin/cbsd jstatus $jailname`

if [ $jstatus -ne "0" ]; then
        /usr/local/bin/cbsd jstop $jailname
        sleep 15
fi

jstatus2=`/usr/local/bin/cbsd jstatus $jailname`

if [ $jstatus2 -eq "0" ]; then
        /usr/local/bin/cbsd jexport jname=$jailname compress=9
        sleep 15
fi

if [ -f $CBSDPATH/export/$jailname.img ]; then
        cp $CBSDPATH/export/$jailname.img $JAILBACKUPTARGET/$jailname-$backupdate.img
        sleep 5
fi

jstatus3=`/usr/local/bin/cbsd jstatus $jailname`

if [ $jstatus3 -eq "0" ]; then
        /usr/local/bin/cbsd jstart $jailname
        sleep 5
fi

jstatus4=`/usr/local/bin/cbsd jstatus $jailname`

if [ $jstatus4 -ne "0" ]; then
        echo "Backup JAIL Finish Successfull! Jail restarted!"
fi

```

This script is launched through **/etc/crontab** For each jail individually according to the schedule.

```
# Backup JAILS monthly
1       4       1       *       *       root    /root/Scripts/backup-jail.sh dokuwiki
1       4       2       *       *       root    /root/Scripts/backup-jail.sh owncloud
1       4       3       *       *       root    /root/Scripts/backup-jail.sh samba && /usr/local/bin/cbsd jexec jname=samba /usr/local/etc/rc.d/transmission stop
1       4       4       *       *       root    /root/Scripts/backup-jail.sh ftpbackup

```

The result of the script is in the directory /data/JAILS/ every month in the first days will be created compressed exports of all cells with all the settingsи.

# Useful alias for the interpreter CSH

When working with cells frequently, it is convenient to shorten some large commands by creating alias in the interpreter settings - for example .cshrc

Viewing Filesystems - Option One.

```
alias df        'df -m | grep -v "\/dev"'

```

Viewing Filesystems - Option Two.

```
alias df2       'df | grep -v "\/jails\/"'

```

List of all jails from all nodes.

```
alias jall      cbsd jls alljails=1 shownode=1

```

List of all nodes and their statuses.

```
alias cnodes    cbsd node mode=list

```

Commands of start and stop torrent of the client of Transmission inside of a jail.

```
alias tstop     '/usr/local/bin/cbsd jexec jname=samba /usr/local/etc/rc.d/transmission stop'
alias tstart    '/usr/local/bin/cbsd jexec jname=samba /usr/local/etc/rc.d/transmission start'

```

Copyright © 2013—2024 CBSD Team.

