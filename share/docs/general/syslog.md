# FreeBSD: syslog and debugging

## syslog

Starting from **CBSD** 11.1.19 You can use the syslog subsystem to collect messages that occur during the process of **CBSD** scripts

The configuration file for the log subsystem: _~cbsd/etc/defaults/logger.conf_. Create new file _~cbds/etc/logger.conf_ to override the default values.

Using the syslog configuration file, you can redirect all messages of **CBSD** in a separate file and in the future use different solutions for the collection and analysis of messages.

_/etc/syslog.d/cbsd.conf_:

```
!cbsd
*.*                     /var/log/cbsd.log

```

And create empty file:

```sh
touch /var/log/cbsd.log
```

After syslog restarting, messages from **CBSD** can be read in a file /var/log/cbsd.log

## debugging

If you encounter an error in the script, you can get a trace of all sh commands executed by running a particular **CBSD** script through the **CBSD\_DEBUG** environment variable, for example:

```sh
env CBSD_DEBUG=1 cbsd jls

```

Copyright © 2013—2024 CBSD Team.

