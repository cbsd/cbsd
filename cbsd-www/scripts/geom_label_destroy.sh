#!/bin/sh

DISK=$1;
GEOM_LABEL=$2;

echo -n "Destroying GEOM label ${GEOM_LABEL}..."
/sbin/glabel destroy ${GEOM_LABEL}
/sbin/glabel clear ${DISK}
RV=$?

if [ "$RV" = "0" ]
then
 echo " done"
fi
exit $RV

