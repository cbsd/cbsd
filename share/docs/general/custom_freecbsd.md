# Modification which are carried out by **CBSD** scripts in FreeBSD

Due to the fact that the course taken by **CBSD** is focused on a large number of functional relationship for the provision of an integrated solution, the system for their work makes or proposes to make a number of specific settings. This page describes where and why these changes are necessary. It is also important to fully uninstall **CBSD** ;-)

## Файл /etc/rc.conf

The following settings in /etc/rc.conf affects by **CBSD** to work correctly.

- 1) _rcshutdown\_timeout_
This parameter adjusts the time of processing the sequence of shutdown rc.shutdown files — if after this timestop the process does not transfer control system would kill him. For servers having a large number of jails, which in turnmay work to stop the long and sensitive to killing services (eg database), the default value (120) is extremely small. In this regard, **CBSD** change it at the first initialization.

If you want to keep the default value and stop the redefine parameter when **cbsd initenv**, simply duplicate the parameter value from /etc/defaults/rc.conf: **CBSD** will not change the setting if in /etc/rc.conf its already sets:


```
  				% grep rcshutdown_timeout /etc/defaults/rc.conf >> /etc/rc.conf

```

- 2) _sshd\_flags and default SSHd port in master node_
As in the case with **rcshutdown\_timeout** in _/etc/rc.conf_, in absence of a _/etc/rc.conf_ parameter **sshd\_flags**, _cbsd initenv_ prompts you to change the port (22) default daemon **sshd** on port 22222 by running it through this setup:


```
  					sshd_flags="-oPort=22222"

```


Be extremely careful about this change. If you want to keep the standard flags sshd daemon, duplicate entry for sshd\_flags from file /etc/defaults/rc.conf in /etc/rc.conf and **CBSD** will not offer change.

Also, you may want to disable host name resolv via DNS, which can significantly speed up connections between nodes, extending the entry in /etc/rc.conf to


```
  					sshd_flags="-oUseDNS=no -oPort=22222"

```


If you want sshd listening port 22222 as well as the standard 22, this record will be correct:


```
  					sshd_flags="-oPort=22 -oPort=22222 -oUseDNS=no"

```


22222 instead of the default port 22 is proposed to change some the following reasons:


  - to this port comes less scanners for sshd bruteforce. Of course, this argument is not serious ;-) and if you want protection from brute force, just look at utilities **security/{bruteblock,denyhosts}**;
  - Ideology of **CBSD** with nodes implies a connection between a server via ssh and it is desirable that a unique/selected port sshd (in this case 22222), always identified **sshd** serving **node**.

In practice, different ports sshd on the master node and jail is useful because if you suddenly forget what kind of node on running your jail **jail1024.my.domain**, you can always be sure that when connecting to the host **jail1024.my.domain** on the 22222 port you will certainly get to the physical node serving this jail. The second point — that the jail with the parameter _oninterface=0_ and have the same basic system with IP. In addition, this setting will prevent port conflicts between a nodes sshd and sshd service in the jail.

Also note that, while parameter **applytpl=1** and the creation of new jail, **CBSD** in the same way configures port in jail to port 2222 (however, sshd\_enable default remains in NO). This change is also driven by a desire to identify jail unique port number. That, in turn, are useful for configuration management systems, such as [**Ansible**](http://www.ansible.com/home), introducing the names of jail in a single domain (eg jail1024. **j**.my.domain) and use the mask of **.j.** in the zone system will apply certain filters to know the correct port for establishing a connection. A similar example of **CBSD** and **Ansible** will be described in a separate article.

In order not to edit scripts on servers which is make ssh connection to node or cells, it is recommended to use the possibility of ~/.ssh/config, where you can specify the ports for a particular host or hosts mask, for example:

```
					# this server on 22 default ssh port
					Host otherserver.my.domain
					Port 22
					User root

					# mask for jails which have records in blabla.j.my.domain zones:
					Host *.j.*
					Port 2222

					# Default records - all host on 22222 port
					Host *
					ControlMaster auto
					ControlPath ~/.ssh/sockets/%r@%h:%p
					Port 22222
					StrictHostKeyChecking no
					UserKnownHostsFile /dev/null

```

- 3) _devfs\_load\_rulesets_

This parameter is needed to initialize the devfs rule. By default, new jail will use **4** ruleset (see /etc/defaults/devfs.rules). However, the default setting devfs\_load\_rulesets in /etc/defaults/rc.conf is set to NO, which in turn will open up in jails all devices in **/dev**, that in the general case — is undesirable. to change the number of rule sets for a specific jail you can via **cbsd jconfig**

4) _cbsdd\_enable, cbsdrsyncd\_enable, cbsdrsyncd\_flags, cbsd\_workdir_

The following entries are required in /etc/rc.conf for correct operation **CBSD**, which proposes to make the installation script initenv:

```
			cbsdrsyncd_enable="YES"
			cbsdrsyncd_flags="--config=/usr/jails/etc/rsyncd.conf"
			cbsdd_enable="YES"
			cbsd_workdir="/usr/jails"

```

cbsddrsyncd is a script which is run rsyncd with an alternative configuration file on port 1873. If you do not plan to transfer jails through **cbsd jcoldmigrate**, you can prefer to turn it off via:

```
			cbsdrsyncd_enable="NO"

```

## /boot/loader.conf file, kernel modules, net.inet.ip.fw.default\_to\_accept

In that case, if you use **cbsd natcfg, cbsd naton** or answer **yes** to the question of using NAT during **cbsd initenv**, the system may leave (with your consent) in your **/boot/loader.conf** the following entries:

```
			pf_load=YES
			ipfw_load=YES
			ipfw_nat_load=YES
			libalias_load=YES
			net.inet.ip.fw.default_to_accept=1

```

depending on whether NAT is used. NAT rules themselves are written in the directory structure **CBSD**.

Params

```
			net.inet.ip.fw.default_to_accept=1

```

role in the **CBSD** not play, but it is included for reasons that IPFW module by default disables all, and at the time of setting **cbsd** some users can not configure ipfw rules (or configure with error), which would entail the loss of access to the system. Keep in mind that this option causes the last rule **65535** as follows:

```
			allow ip from any to any

```

More: [FreeBSD handbook::Firewalls-ipfw](http://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/firewalls-ipfw.html)

In addition, at the start of NAT ( **cbsd naton**), this call is made in system:

```
			% sysctl -qn net.inet.ip.forwarding=1
			% sysctl -qn net.inet6.ip6.forwarding=1

```

for the correct functioning of NAT, and at start of **vnet**-jails (VIMAGE), **if\_bridge** module will be automatically loaded.

Note that **CBSD** does not remove a record from a /boot/loader.conf on deinstall, or for example, if you switch from one NAT framework to another (for example, first-configured PF, then switched to IPFW), then module in loader.conf for pf is remain.

## /usr/local/etc/sudoers.d/cbsd\_sudoers file

Most of the **CBSD** commands requires superuser role, and therefore the use utility **sudo** and the follow configuration file /usr/local/etc/sudoers.d/cbsd\_sudoers:

```
			Defaults     env_keep += "workdir DIALOG NOCOLOR"
			Cmnd_Alias   CBSD_CMD = /usr/jails/sudoexec/*,/usr/local/cbsd/sudoexec/*
			cbsd   ALL=(ALL) NOPASSWD: CBSD_CMD

```

These records can run scripts with root authority in the following directories:

```
			/usr/jails/sudoexec/
			/usr/local/cbsd/sudoexec/

```

and without the need to enter a password **root**

## IPFW rule for jail traffic counting

Currently, if the question **Enable IPFW** is affirmative and IPFW enabled on the system, ipfw will be used for traffic counters of jail when it starts and stopping. In this case, the range of rules for the installation of meters fixed in the file $workdir/cbsd.conf. By default, its range between 99 and 2000:

```
		fwcount_st = "99"
		fwcount_end = "2000"

```

In this latest rule (2000) will be used to set NAT rules, if IPFW selected as a NAT. Accordingly, if you are planning to write their own rules, you need to exclude this range (or adjust it in the configuration file) to go after other rules counters (> fwcount\_end). Currently, looking for an alternative way to calculate the traffic jail.

Copyright © 2013—2024 CBSD Team.

