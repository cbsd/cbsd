# default CBSD pf.conf
## You can overwrite this via ~cbsd/etc/pf.conf

## include NAT rules
include "%%CBSD_WORKDIR%%/etc/pfnat.conf"

# or:
# nat-anchor "/usr/jails/etc/pfnat.conf"


## include RDR rules
include "%%CBSD_WORKDIR%%/etc/pfrdr.conf"

# or:
# rdr-anchor "/usr/jails/etc/pfrdr.conf"
