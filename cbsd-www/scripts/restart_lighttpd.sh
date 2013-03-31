#!/bin/sh

# sleep 5 seconds
sleep 5

# restart lighttpd
/usr/local/etc/rc.d/lighttpd restart
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

