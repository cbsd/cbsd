## default CBSD pf.conf
## You can overwrite this via ~cbsd/etc/pf.conf
# additional flags sample, e.g:
# scrub fragment reassemble
# scrub in all fragment reassemble max-mss 1440
# ..
# set skip on lo0
# scrub in all
# block in all
# pass out all keep state

## include NAT rules
include "%%CBSD_WORKDIR%%/etc/pfnat.conf"

# or:
# nat-anchor "/usr/jails/etc/pfnat.conf"


## include RDR rules
include "%%CBSD_WORKDIR%%/etc/pfrdr.conf"

# or:
# rdr-anchor "/usr/jails/etc/pfrdr.conf"
