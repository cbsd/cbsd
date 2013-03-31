#!/bin/sh

DISK=$1;

echo -n "Formatting disk ${DISK}..."
/bin/dd if=/dev/zero of=/dev/${DISK} bs=1m count=1
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

