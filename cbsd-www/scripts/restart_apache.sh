#!/bin/sh

# sleep 5 seconds
sleep 5

# restart apache
/usr/local/etc/rc.d/apache restart
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

