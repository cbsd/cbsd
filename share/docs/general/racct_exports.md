# CBSD: export RACCT metrics

## Intro

wip/draft

Export metrics to beanstalk highly experimental and not enabled by default. So no, beanstalkd at the moment is optional ( and probably will remain so ). Another backend for metrics is SQLite3 (already done) and Prometheus ( still wip ).


If you with to play with metrics enable cbsd racct services:

- for bhyve metrics:




```
sysrc cbsd_statsd_bhyve_enable=YES
```







```
service cbsd-statsd-bhyve start
```

- for jail metics:




```
sysrc cbsd_statsd_jail_enable=YES
```







```
service cbsd-statsd-jail start
```

- for hoster metics ( **required by DRS**):




```
sysrc cbsd_statsd_hoster_enable=YES
```







```
service cbsd-statsd-hoster start
```


Also you need to enable/install beanstalkd:

```
pkg install -y net/beanstalkd
```

```
sysrc beanstalkd_enable=YES
```

```
sysrc beanstalkd_flags="-l 127.0.0.1 -p 11300"
```

```
service beanstalkd start
```

To receive real-time metrics use 'racct-jail' tube in beasntalkd to receive metrics for jail (in json), 'racct-bhyve' tube for bhyve and 'racct-system' for hoster metrics

update rate and other settings are in the corresponding configuration file in _~cbsd/etc/default_ directory ( **racct-{jail,bhyve,hoster}-statsd.conf**). Please use _~cbsd/etc_ directory
for overwrite default settings.

E.g. simple python-based client:

```
pkg install -y net/py-beanstalkc   (install client)
```

```
#!/usr/bin/env python2.7

import beanstalkc
#import time
import sys

try:
    bsk = beanstalkc.Connection(host='localhost', port=11300)
    bsk.watch('racct-jail')
    bsk.use('racct-jail')
except:
    print "cannot open connection"
    sys.stdout.flush()
    sys.exit(1)

while 1:
    #print time.strftime('%Y-%m-%d %X'), " waiting for work ..."
    sys.stdout.flush()
    job = bsk.reserve(timeout=60)
    if job is not None:
        # I had to delete it immediately
        job.delete()
        try:
            sys.stdout.flush()
            print(repr(job.body))
        except:
            print "bad things"
            sys.stdout.flush()

```

Copyright © 2013—2024 CBSD Team.

