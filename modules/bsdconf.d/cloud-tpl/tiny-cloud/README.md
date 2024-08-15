olevole hint: looks like tiny-cloud/nocloud too limited at the moment: no network config
( via user-data as #!/bin/sh scenario ) + no user management (alpine is hardcoded) --
pubkey via meta-data only.

Stages:

0) tiny-cloud boot

   ++ install_hotplugs: starting
   ++ install_hotplugs: done
   ++ set_ephemeral_network: starting
   ++ set_ephemeral_network: done
   ++ set_default_interfaces: starting
set_default_interfaces: already set up
   ++ set_default_interfaces: done
   ++ enable_sshd: starting
 * rc-update: sshd already installed in runlevel `default'; skipping
 * Caching service dependencies ...                                                                                                                   [ ok ]
   ++ enable_sshd: done

1) tiny-cloud early

# olevole notes: early stage always set DHCP, hardcoded >/etc/network/interfaces:
```
auto eth0
iface eth0 inet dhcp
```

   ++ save_userdata: starting
   ++ save_userdata: done


2) tiny-cloud main

// missing_userdata: no user-data found ?

yx -f "/var/lib/cloud/meta-data" hostname
yx -f "/var/lib/cloud/meta-data" public-keys
yx -f "/var/lib/cloud/meta-data" public-keys 1 openssh-key
yx -f "/var/lib/cloud/meta-data" public-keys 2 openssh-key


3) tiny-cloud final

   ++ bootstrap_complete: starting
   ++ bootstrap_complete: done




STATIC sample

iface eth0 inet static
        address 192.168.1.150
        netmask 255.255.255.0
        gateway 192.168.1.1
