# **CBSD** integration with PHPIPAM.

## Intro

With an increase in the number of working virtual machines and containers, the problem of inventory and accounting of the address space used may occur.
In addition, if the cluster consists of two or more physical hosters, there is a problem of obtaining unique IP addresses between nodes for guest environments.
For these purposes, some external /micro/ service is suitable for accounting IP addresses.


This article will demonstrate the possibilities of external **CBSD** hooks on the example of the cluster of three physical servers located in different DC,
but organizing a single VPC for virtual environments based on VXLAN and integration with the IP Management system called [PHPIPAM](https://phpipam.net/).


We assume that **CBSD** nodes are already configured and between them is organized by VPC, as described in the
[VPC with **CBSD** (vxlan)](http://www.convectix.com/en/13.0.x/wf_vpc_ssi.html) article.


## Installing PHPIPAM

Install PHPIPAM using any suitable way to choose from: PHPIPAM can be installed from ports:


```sh
make -C /usr/ports/net-mgmt/phpipam install
```

or via pkg:


```sh
pkg install -y phpipam
```

, or from [official repositories](https://github.com/phpipam/phpipam) on GitHub.


In addition, for **CBSD** there is [CBSDfile](http://www.convectix.com/en/cbsdfile.html) to deploy this service from scratch through the "cbsd up" command.
In this example, we will follow the rapid/quick way and use a ready-made image for the **CBSD**
an image that is the result of the "cbsd jexport" command to the container formed by [CBSDfile](https://github.com/cbsd/cbsdfile-recipes/blob/master/jail/phpipam/CBSDfile)).
In our presence there are three servers with names: SRV-01, SRV-02 and SRV-03. We choose any of them as a hoster for phpipam and get a container:


```sh
cbsd repo action=get sources=img name=phpipam
```

![](http://www.convectix.com/img/phpipam/phpipam1.png)

Run container:


```sh
cbsd jstart phpipam
```

![](http://www.convectix.com/img/phpipam/phpipam2.png)

Alternative via CBSDFile:


```sh
cd /tmp
git clone https://github.com/cbsd/cbsdfile-recipes.git
cd cbsdfile-recipes/jail/phpipam
cbsd up
```

(If necessary, to build for alternative version of FreeBSD, through the **ver** argument: cbsd up **ver=12.2**)


And open the page in the browser: http://<container IP>


![](http://www.convectix.com/img/phpipam/phpipam3.png)

## PHPIPAM setup

Authorizes in PHPIPAM as administrator using default credential:


- login: **Admin**
- password: **ipamadmin**

Change the password (in our case, we set the password to 'qwerty123') and activate the functionality of the API to work with PHPIPAM remotely. To do this, click on the 'phpIPAM settings' item:


![](http://www.convectix.com/img/phpipam/phpipam4.png)

Set the Site URL if necessary: to the correct value. This is especially important if the service works through external balancer. If you use the NGINX-based balancer, make sure that the configuration pass the corresponding headers:



	location / {
		proxy_pass http://:80;
		proxy_set_header Host $host;
		proxy_set_header X-Real-IP $remote_addr;
		proxy_set_header X-Forwarded-Host $host;
		proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
		proxy_set_header X-Forwarded-Proto $scheme;
	}

Activate API features, do not forget to save the changes via **save** button then go to the **API** settings through the left menu:


![](http://www.convectix.com/img/phpipam/phpipam5.png)

Create a key to access API:


![](http://www.convectix.com/img/phpipam/phpipam6.png)

As an **App id**, use an arbitrary unique identifier (which you need to remember to configure the **CBSD** module). In our case, we use ID: **Admin**

Set the access rights to the value: **Read/Write**

Set the access method to the API through the token: **User token**

![](http://www.convectix.com/img/phpipam/phpipam7.png)

Also, through the Subnets menu, we must add/create a working network that is given to virtual environments and which will be notified through API.


![](http://www.convectix.com/img/phpipam/phpipam8.png)

![](http://www.convectix.com/img/phpipam/phpipam9.png)

In this example, this network is **CBSD** VPC1: **10.0.1.0/24**

![](http://www.convectix.com/img/phpipam/phpipam10.png)

This PHPIPAM configuration is completed.

## Installing IPAM module for CBSD

Obtain and activate the IPAM module for **CBSD** (ATTENTION, the **CBSD** version must be no less than 13.0.4).


```sh
cbsd module mode=install ipam
echo 'ipam.d' >> ~cbsd/etc/modules.conf
cbsd initenv
```

Copy the standard configuration file and adjust the credentil:


```sh
cp -a /usr/local/cbsd/modules/ipam.d/etc/ipam.conf ~cbsd/etc
vi ~cbsd/etc/ipam.conf
```

In our case, PHPIPAM works at http://10.0.1.7, so the configuration file _~cbsd/etc/ipam.conf_ will look like this:



	PHPIPAMURL="http://10.0.1.7"
	PHPIPAMURLAPI="${PHPIPAMURL}/api"
	USER="admin"
	PASS="qwerty123"
	APPID="Admin"
	DEBUG=0
	# PHPIPAM APP Security ( only 'token' is supported at the moment )
	APP_SECURITY="token"


You can get acquainted with the operations that IPAM module provides for the **CBSD** through the 'cbsd ipam --help' command. As we see, the possibilities cover such operations as:


- 1) Request for the first free IP address;
- 2) Creating and/or updating data for the specified IP address;
- 3) Delete IP address;

These three actions will be used as a 'cbsd dhcpd' script that offers a free IP address for jail and virtual machines, as well as in **create/stop/start/destroy** hooks.
As a check, that PHPIPAM + phpipam module are configured correctly, you can try to create and delete any test record via CLI, for example:


```sh
cbsd ipam mode=create subnet=10.0.1.0/24 ip4_addr=10.0.1.50 description="jail" note="srv-01.my.domain" hostname="jail1.my.domain" debug=1
```

If the record was created in PHPIPAM, then you are left very little - politely ask the **CBSD** to do it for you, further ;-)


![](http://www.convectix.com/img/phpipam/phpipam11.png)

To remove our test record:


```sh
cbsd ipam destroy
```

## CBSD setup

_Attention! All of the following settings should be done equally on all cluster servers,_
_so it is recommended to use any convenient config-managent tools like **Puppet, SaltStack, Chef or Ansible.**._
_For clarity, actions for one host are described here directly through CLI/shell._

**a)** We will need to reconfigure the \`cbsd dhcpd\` to use an external script.


To do this, copy the default configuration file dhcpd.conf and change the 'internal' value to the external script
that will work with the PHPIPAM. For example, copy this file as _/root/bin/phpiapm.sh_:


```sh
cp ~cbsd/etc/defaults/dhcpd.conf ~cbsd/etc/
vi ~cbsd/etc/dhcpd.conf
```

Example:


	dhcpd_helper="/root/bin/dhcpd-ipam"


Create a /root/bin directory and put a script in it that calls the first\_free method, to obtain the first free IP from PHPIPAM.


The call example is here: _/usr/local/cbsd/modules/ipam.d/share/dhcpd/dhcpd-ipam_:


```sh
mkdir /root/bin
cp -a /usr/local/cbsd/modules/ipam.d/share/dhcpd/dhcpd-ipam /root/bin/
```

Edit the subnet= argument in /root/bin/dhcpd-ipam to the network that you use for virtual environments (and configured in PHPIPAM).


In our case, this is **10.0.1.0/24**, respectively, the script will be the following:



	#!/bin/sh

	cbsd ipam mode=firstfreelock subnet=10.0.1.0/24

**b)** copy the scripts that will be launched as create/destroy/start/stop hooks of environments.
Examples of these scripts are here: _/usr/local/cbsd/modules/ipam.d/share_.


```sh
mkdir -p /root/share/cbsd-ipam
cp -a /usr/local/cbsd/modules/ipam.d/share/*.d /root/share/cbsd-ipam/
```

In /root/share/cbsd-ipam now we have three directories on the name of the directories that work out in **CBSD** at certain events:


- **master\_create.d** \- works when creating a new environment;
- **master\_poststart.d** \- works after starting the environment;
- **remove.d** \- works when destroying the environment;

Inside each directory, the executed **ipam.sh** file exist, which you need to adjust the working network to be operated in PHPIPAM.
In our case, this is **10.0.1.0/24**.
If your network is different, for example, **192.168.0.0/16**, then replace in all scripts:


**1)** change string **10.0.1.\*)** in **case** condition to:

192.168.\*)

**2)** change value for **subnet** argument in 'cbsd ipam' string, to your network:

subnet=192.168.0.0/16


Of course, you can write your own, more elegant handler instead of these demonstration scripts.
Now, if you do not use [your own profiles](http://www.convectix.com/en/13.0.x/wf_profiles_ssi.html), just link the scripts to the cbsd directories.


For jail:

```sh
ln -sf /root/share/cbsd-ipam/master_create.d/ipam.sh ~cbsd/share/jail-system-default/master_create.d/ipam.sh
ln -sf /root/share/cbsd-ipam/master_poststart.d/ipam.sh ~cbsd/share/jail-system-default/master_poststart.d/ipam.sh
ln -sf /root/share/cbsd-ipam/remove.d/ipam.sh ~cbsd/share/jail-system-default/remove.d/ipam.sh
```

For bhyve:

```sh
ln -sf /root/share/cbsd-ipam/master_create.d/ipam.sh ~cbsd/share/bhyve-system-default/master_create.d/ipam.sh
ln -sf /root/share/cbsd-ipam/master_poststart.d/ipam.sh ~cbsd/share/bhyve-system-default/master_poststart.d/ipam.sh
ln -sf /root/share/cbsd-ipam/remove.d/ipam.sh ~cbsd/share/bhyve-system-default/remove.d/ipam.sh
```

That's all! Now, working with a CBSDfile or API, or CLI, by creating and deleting jail or bhyve virtual environments on any of the three servers, you solve the problems of:


- possible collisions when issuing free addresses, since now this logic is beyond local **CBSD**
installations into a separate entity/service that guarantees issuing free IP;
- automatic address space accounting and documentation, which virtual environment ( **name**), what emulator type ( **jail/Bhyve**)
has taken one or another IP and on which node it is started (the **description** field in PHPIPAM will be filled with the **CBSD** on which the environment is launched );

[![](http://www.convectix.com/img/phpipam/phpipam12.png)](http://www.convectix.com/img/phpipam/phpipam12.png)

**Good luck, we wish the passing wind and light clouds!**