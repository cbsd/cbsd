# An example of **CBSD** integration with MONIT (health-check)

## Intro

If you look at the [list of jail management software](https://wiki.freebsd.org/Jails#Jail_Management_Tools) on FreeBSD,
you will probably notice that there is no shortage of such utilities.
Various jail wrappers (also relevant for bhyve/XEN) provide a wide variety of formats for directive, command and argument entries to suit all tastes.
However, they all (including **CBSD**, of course) basically offer the same operations and capabilities with minor differences,
namely basic 'create', 'delete', 'start' and 'stop' of environments.
But nobody goes further: higher-level entities like controller, supervisor, health-check, DRS, etc. - absent as a class.

Without modifications, this limits the range of application of jail/bhyve wrappers to the scale of a localhost,
due to the lack of the ability to transparently use a deployment to a group of physical nodes.

There is a fairly logical explanation for this - in modern realities it is rather rash to try to solve all the problems of the universe with one product.
That is why **CBSD** places great emphasis on integration capabilities with tools that extend the capabilities of the CBSD framework to provide additional capabilities to users.


In this chapter, we solve the issue of monitoring the health of services in containers or virtual machines using [monit](http://www.convectix.com/_blank),
followed by restarting the environments under certain conditions.
The main actor providing the integration is the export of **CBSD** environment facts, which are dynamic data,
and the presence of [**hooks**](http://www.convectix.com/en/13.0.x/wf_jconfig_ssi.html#execscript) in **CBSD**,
which automates the process of creating and deleting **monit** rules.


Other chapters in this series:


- [API module: private cloud via API](http://www.convectix.com/en/cbsd_api_ssi.html)
- [VPC with **CBSD** (vxlan)](http://www.convectix.com/en/13.0.x/wf_vpc_ssi.html)
- [**CBSD** integration with PHPIPAM (IP management)](http://www.convectix.com/en/13.0.x/wf_ipam_ssi.html)
- [Example of using CBSD/bhyve and ISC-DHCPD](http://www.convectix.com/en/articles/cbsd_vm_hook_dhcpd.html)
- [Example of using CBSD/jail and Consul](http://www.convectix.com/en/articles/cbsd_jail_hook_consul.html)
- DRS for **CBSD**

Why **Monit**?
When in 2021 we talk about the automatic restart of containers depending on certain conditions, such monstrous Linux-centric solutions as OpenShift, Kubernetes, Systemd... immediately come to mind.


The FreeBSD camp will likely be closest to using a product from **[HashiCorp](https://www.hashicorp.com/)** called
[Consul](https://www.hashicorp.com/products/consul). **HashiCorp** products are famous for their extremely good integration with each other:
everyone knows a gang of bosom friends: [Nomad](https://www.hashicorp.com/products/nomad),
[Consul](https://www.hashicorp.com/products/consul) and [Vault](https://www.hashicorp.com/products/vault).


This is a good and high-quality modern stack, but we will choose the path of minimalism and simplicity,
where our task is not to choose according to the principle of "what everyone is talking about",
but to get the cheapest and easiest solution to the problem of monitoring and restarting services.

For these tasks, the **monit** has enough capabilities.
To assess the difference, just look at the difference in the consumed resources of both services:


monit consul executable file size: 425 Kb**78 Mb**RAM consumption without check-rules (default):25 Mb **80 Mb**

[![](http://www.convectix.com/img/monit/monit1.png)](http://www.convectix.com/img/monit/monit1.png)

[![](http://www.convectix.com/img/monit/monit2.png)](http://www.convectix.com/img/monit/monit2.png)

In addition, the purpose of the article is to show the concept itself, which can be easily used with any other, more massive tools.


## Idea and architecture

**monit** works according to the [monitrc](https://mmonit.com/monit/documentation/monit.html) configuration file,
which lists the rules for checking certain events and the reaction to them.
Our task is to add rules at runtime when creating a container,
if the container has something to check.
And accordingly, remove from monitoring when the container is stopped or removed.
In response to a failed test, force **monit** to reload the container.
This means that each physical **CBSD** host will run its own unique instance of monit,
which only works with local environments.
Of course, we can get by with one **monit**, which will check everything,
but this will be a single-point-of-failure, which will require additional labor
to ensure the reliability of the external **monit** service itself.


We will maintain the configuration of checks for each environment,
regardless of the general configuration, so that the rules always remain with the environment, even if it moves from node to node.
This is _especially_ useful when using [**CBSDfile**](http://www.convectix.com/en/cbsdfile.html) deployments,
where rules and hook scripts can be located in the same directory.


In this example, we can check the availability of a TCP port or certain content via the HTTP protocol,
therefore, of all the dynamic parameters, the **monit** process will need only two from **CBSD** \-
the name of the container (for restarting) and the IP address, the port availability on which will be checked.
To do this, let's prepare a template in the _~cbsd/jails-system/JNAME/monit/_ directory from which,
when starting, stopping or deleting, the working monit configuration will be generated or deleted,
and when updated, the main **monit** process must re-read it.
The simplest implementation as an example is implemented using shell scripting
as a module of the [same name](https://github.com/cbsd/modules-monit) for **CBSD**.


## installing and configuring monit

Install **monit** via pkg:

```
 pkg install -y monit

```

Copy the example configuration file into the working one and add at the end the directive to include all configuration files
in _~cbsd/jails-system/\*/monit/monitrc_ \- they will be generated by the module.
Please note that _/usr/jails_ (the **CBSD** working directory) may be different in your case:



```
 cp -a /usr/local/etc/monitrc.sample /usr/local/etc/monitrc
 echo 'include /usr/jails/jails-system/*/monit/monitrc' >> /usr/local/etc/monitrc

```

## Configuring CBSD

Let's install the module as described on the [project page](https://github.com/cbsd/modules-monit):


```
  cbsd module mode=install monit
  echo 'monit.d' >> ~cbsd/etc/modules.conf
  cbsd initenv

```

Let's copy the scripts that will be launched as destroy/start/stop of environment hooks.
Examples of these scripts can be found here: _/usr/local/cbsd/modules/monit.d/share_.


```
mkdir -p /root/share/cbsd-monit
cp -a /usr/local/cbsd/modules/monit.d/share/*.d /root/share/cbsd-monit/

```

In _/root/share/cbsd-monit/_ we now have three directories named by directories, which are processed in
**CBSD** on certain events:


- **master\_poststart.d** \- works after starting the environment;
- **master\_create.d** \- works when creating a new environment;
- **remove.d** \- works when destroying the environment;

Inside each directory is an executable file called **monit.sh**, which will do all the work.


Of course, you can write your own, nicer handler instead of these demo scripts.
Now, if you are not using [your own profiles](http://www.convectix.com/en/13.0.x/wf_profiles_ssi.html),
just link the scripts to the **CBSD** directories:


for jail:

```
ln -sf /root/share/cbsd-monit/master_poststart.d/monit.sh ~cbsd/share/jail-system-default/master_poststart.d/monit.sh
ln -sf /root/share/cbsd-monit/master_prestop.d/monit.sh ~cbsd/share/jail-system-default/master_prestop.d/monit.sh
ln -sf /root/share/cbsd-monit/remove.d/monit.sh ~cbsd/share/jail-system-default/remove.d/monit.sh

```

for bhyve:

```
ln -sf /root/share/cbsd-monit/master_poststart.d/monit.sh ~cbsd/share/bhyve-system-default/master_poststart.d/monit.sh
ln -sf /root/share/cbsd-monit/master_prestop.d/monit.sh ~cbsd/share/bhyve-system-default/master_prestop.d/monit.sh
ln -sf /root/share/cbsd-monit/remove.d/monit.sh ~cbsd/share/bhyve-system-default/remove.d/monit.sh

```

When starting the container, the script checks for a template in the system environment directory: _~cbsd/jails-system/JNAME/monit/monitrc.tpl_.
If there is one, sed is used to replace the signatures %%JNAME%% and %%IPV4\_FIRST%% in the template for the values of the fact variables ${jname}
and ${ipv4\_first}, creating the resulting file _~cbsd/jails-system/JNAME/monit/monitrc_.
This file is processed through the include we added from the main configuration file _/usr/local/etc/monitrc_.
Of course, you may want to add and use other facts as needed.
In addition to generating the configuration file, the scripts reload the service to apply the configuration.


This completes the configuration of **monit** and **CBSD**!
It remains for us to create a configuration for **monit** in the system directory to check certain events,
using the standard [monit documentation](https://mmonit.com/monit/documentation/monit.html).


## Example

Let's create a container named **lb1**,
which is supposed to use an nginx-based WEB server as load-balancer.
**monit** will be configured to check the opened 80/tcp port and if the nginx service
for some reason does not serve this port, we will force **monit** to reload the container.


Let's create a container with nginx:

```
 cbsd jcreate jname=lb1 pkglist=nginx sysrc=nginx_enable=YES

```

Let's copy the example to check and restart the container without any modifications to the
_~cbsd/jails-system/lb1/monit/_ directory:


```
 mkdir ~cbsd/jails-system/lb1/monit/
 cp -a /usr/local/cbsd/modules/monit.d/share/monitrc.tpl ~cbsd/jails-system/lb1/monit/

```

Let's start the container and check the **monit** configuration via 'monit status'.
The service must have a task with the container name **lb1**:


```
 cbsd jstart lb1
 monit status

```

![](http://www.convectix.com/img/monit/monit3.png)

Now simulate a denial of nginx service, you just need to stop it:

```
cbsd jexec jname=lb1 service nginx stop

```

and watch what happens next.

**Good luck, we wish the passing wind and light clouds!**

Copyright © 2013—2024 CBSD Team.

