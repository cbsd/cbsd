#!/bin/sh

DISK=$1;

echo -n "Random Writing disk ${DISK}..."
/bin/dd if=/dev/urandom of=/dev/${DISK} bs=1m
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

