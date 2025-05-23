**CBSD** was designed to be user-friendly by providing the convenience of interactive dialogs. But what about building a scalable cluster? **CBSD** can be useful in this case as well.

This article describes an example of creating and managing a **CBSD** cluster via an asynchronous interface using the minimalistic and fast [net/beanstalkd](http://xph.us/software/beanstalkd/) broker. Any other broker can be used in place of **beanstalk**, such as ActiveMQ, ZeroMQ, RabbitMQ or Kafka. 

By convention, let's call this low level virtual machine management. **CBSD** provides an interface for tasks involved in managing virtualized services, such as creating a VM, managing storage, creating snapshots, vm and jail migration, vm and jail cloning, managing VNC, etc. You can use **CBSD** directly or use a higher level application such as a gui or web interface with **CBSD** acting as an intermediary or glue layer.

We will create multiple **CBSD** workspaces, with resources initialized in separate directories. This creates an opportunity for building pool-bound methods for hosting virtual machines. A pool-bound cluster is where all services or virtual machines of the cluster will be tied to one or another in a managment pool, which can be moved from one server to another in emergency situations, during DRS operations or during equipment maintenance. Thus, it can become the basis for building a shared-nothing cluster based on FreeBSD and managed by **CBSD**.
![](http://www.convectix.com/img/cbsd_pool_mq1.png)

The creation of a failover cluster will be described in a separate, more detailed article. Here we will discuss a scenario with several **CBSD** working environments to demonstrate the operation of the asynchronous interface through a broker bus. We assume that **CBSD** is already installed and configured on the server. We need a **beanstalkd** service that acts as a shared bus for all agents. Let's put **beanstalkd** in a jail on our server. To do this, create a jail with an arbitrary name in which **beanstalkd** will be launched, for this example, **bs1** (assign the container a working IP address).

![](http://www.convectix.com/img/cbsd_pool_mq2.png)

```sh
cbsd jconstruct-tui
cbsd pkg jname=bs1 mode=update
cbsd pkg jname=bs1 mode=install net/beanstalkd
cbsd sysrc jname=bs1 beanstalkd_enable=YES
cbsd jstart bs1
```

Then, initialize two independent environments (in a real cluster, these can be different pools and, of course, there may be more), for example, in /pool1 and /pool2 directories:

```sh
env workdir=/pool1 /usr/local/cbsd/sudoexec/initenv
```

- Answer no to the question of changing the rc.conf file, this initialization should not modify your host configuration files.

- Answer no to the question of enabling NAT (nat\_enable: Enable NAT for RFC1918 networks?). NAT should already be configured correctly on the host system.

Repeat the same for the second environment:

```sh
env workdir=/pool2 /usr/local/cbsd/sudoexec/initenv
```

Now, **CBSD** can work in these environments through the workdir variable, for example:

```sh
env workdir=/pool1 cbsd jconstruct-tui
env workdir=/pool2 cbsd jconstruct-tui
```

Each environment will be served by a lightweight agent (let's call it bs\_router) which will connect to beanstalkd and process requests. Clone bus router:

```sh
cd /root
git clone https://github.com/cbsd/bs_router.git /root/bs_router
```

This example is written in GO, so to build the project we need to install golang:

```sh
pkg install -y lang/go
```

Build:

```sh
cd bs_router
setenv GOPATH /root/bs_router
go get
go build
cp -a bs_router /usr/local/sbin
```

Now copy the configuration files:

```sh
cp -a config.json /usr/local/etc/pool1.json
cp -a config.json /usr/local/etc/pool2.json
```

In both configuration files change the following variables:

- **uri** \- instead of 127.0.0.1:1130, set IP address of bs1 jail, e.g: **172.16.0.3**:1130 (if bs1 has IP 172.16.0.3)
- **cbsdenv** \- for pool1.json config it will be pointed to /pool1, for pool2.json - /pool2
- **tube** \- which pipe to subscribe to, for pool1.json config let it be "cbsd\_pool1", and for pool2.json - cbsd\_pool2
- **reply\_tube\_prefix** which pipe do we use for reply. For pool1.json let it be: cbsd\_pool1\_result\_id, and for pool2.json - cbsd\_pool2\_result\_id

Now start both agents with the specifying the absolute path to the configuration file:

```sh
/usr/local/sbin/bs_router -config /usr/local/etc/pool1.json
/usr/local/sbin/bs_router -config /usr/local/etc/pool2.json
```

That's it! Now everything that we will send to the beanstalk queue with the corresponding name and the corresponding payload in json format will be transmitted to **CBSD** and a response will be received.

As an example, we can clone a client sample to our CBSD agent, which will connect to beanstalkd and send requests:

```sh
cd /root
git clone https://github.com/cbsd/bs_router-client.git
cd bs_router-client
setenv GOPATH /root/bs_router-client
go get
go build
```

This will build the **bs\_router-client** binary file, which can now be used to send and receive tasks to different **CBSD** environments. Take a look at the bin.jail and bin.bhyve directories for examples of use.

When working with cloud images, it makes sense to first 'warm up' (download) all the cloud images to speed creation of the first virtual machine. For example, for pool1 this can be done like this:

```sh
env workdir=/pool1 cbsd fetch_iso keepname=0 conv2zvol=1 cloud=1 dstdir=default
```

After this command, you can use the result of executing the scripts /root/bs\_router-client/bin.bhyve/bcreate.sh and /root/bs\_router-client/bin.bhyve/bstart.sh without delay.

