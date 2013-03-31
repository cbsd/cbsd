#!/bin/sh

DISK=$1;
GEOM_LABEL=$2;

echo -n "Creating GEOM label ${GEOM_LABEL}..."
/sbin/glabel label ${GEOM_LABEL} ${DISK}
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

