#!/bin/sh

URL=$1;

echo "Downloading system version at ${URL}"
/usr/bin/fetch -o /tmp/ "${URL}" & 2>&1
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

