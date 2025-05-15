# taskd

## Description

Starting with version 10.0.6, in **CBSD** is functional, allowing you to create all single tasks to perform (usually deferred for a while or just long).

Partially, this functionality uses itself **CBSD** for making their own internal problems for the local or remote node, however, you can also use this feature.

The storage queue of tasks and their parameters is SQLite3 database. You can configure different behavior and processing tasks:

- Specify the user who will run
- Specify the remote node. In this case, **CBSD** automatically enters task taskd-queue remote server and will monitor its implementation.
- Specify an alternate logfile or disable logging altogether.
- Get result of working on the problem Email.
- Configure log rotation policy (delete the log automatically if code execution - 0 successfully, delete the log automatically in any case, delete the log records automatically after X or N aging time).
- Indicate dependence (chain) of a problem from the other - a particular task will be located in _Pending_ until another dependence.

For a list of the current task queue, use the command **cbsd taskls**:

```
root@dev:~ # cbsd taskls
ID  STATUS      OWNER  CMD                               LOGFILE           ERRCODE
1   Complete    root   cd /usr/ports && svn up           /tmp/taskd.1.log  32512
2   Complete    root   cbsd repo action=get sources=base /tmp/taskd.2.log  0
3   Complete    root   cbsd svnup                         /tmp/taskd.3.log  0
4   Complete    root   cbsd buildworld                   /tmp/taskd.4.log  0
5   Complete    root   cbsd installworld                 /tmp/taskd.5.log  0
6   Complete    root   cd /usr/ports && svn up           /tmp/taskd.6.log  0
7   In Progress root   cbsd j2slave jname=owncloud       /tmp/taskd.7.log  0

```

where:


- **ID** \- serial number and unique tasks assigned by the system when entering
- **STATUS** \- can have 3 states: _Pending_ (waiting to be processed), _In progress_ \- during operation, _Complete_ \- task is completed
- **OWNER** \- who set the task. Can contain either the user name or the name of the remote node that set the task on this server
- **CMD** -directly task
- **LOGFILE** \- log mining tasks by default stored in /tmp/taskd. **$ID**.log. You can specify when you create an alternate Job log on or off at all
- **ERRCODE** \- code of completed tasks

Tasks are run in parallel, but a configuration file, _$etcdir/taskd.conf_ is governed by a limit on the number of concurrent tasks that the system can perform simultaneously.

New tasks are started immediately upon receipt, however, if the limit is exceeded concurrent tasks, tasks of the latter will be found in the status _Pending_ waiting their turn.

through the parameter **after** in the formulation of a problem, you can specify the JOB ID of the task before execution of which a new task will not run. In addition, upon completion of the new task
dependent, will start only if the code is complete the task dependencies - **0** (worked without error), otherwise the new task inherits **errcode** and will not be activated.
This functionality is useful when you need to perform a number of interdependent tasks sequentially, eg:


```
cbsd task mode=new cbsd svnup    //run the update source
cbsd task mode=new after=1 cbsd world //Builds the world after working svnup, if it job=1
cbsd task mode=new after=2 cbsd kernel // Builds core after working world, if it job=2
cbsd task mode=new cbsd jcoldmigrate jname=jail1 node=n1.node.org // run cold migration for jail1 to a node n1.node.org
cbsd task mode=new cbsd after=5 jremove jail1 // remove slave-jail jail1 from current node, if migration (job 5) is success.

```

ID job-and a new task can be derived from SQL, or process output when the problem - tied her ID output to stdout.

You can configure the notification by email for their tasks through a file _$etcdir/taskd.conf_, overwriting the default settings of _$etcdir/defaults/taskd.conf_

```
root_rcpt="root@localhost"                       # default contact for mail notification or root owner
lastoutput_num="10"                             # how many last strins from the log send to mail
max_simul_jobs="10"                             # max run simultaneous proccesses of nexttask
loop_timeout="60"                               # timeout between forced checking the queue if no events, in seconds

```

where:


- **root\_rcpt** \- contact email (separated by commas for multiple) which will be notified upon completion of the task. For example, if you want to receive email for a certain letter on the tasks, owner which cbsdweb, you must add an entry: cbsdweb\_rcpt="you@email.here"
- **lastoutput\_num** \- insert in a letter last **N** lines task performed.
- **max\_simul\_jobs** \- limit the number of parallel tasks, if the number of tasks is exceeded - the last task will wait for the first execution.
- **loop\_timeout** \- interval between the forced checking queue new challenges.

In fact the task you will receive letters similar to the following:

```
----------  Forwarded Message  ----------
Subject: CBSD dev.my.domain: task 6 is completed
Date: Thursday 29 May 2014, 00:54:06
From: Charlie Root
To: your_email@my.domain

cmd: cd /usr/ports && svn up
runtime: 14 minutes
errcode: 0

last 10 lines of task log (/tmp/taskd.6.log):

Restored 'audio/zynaddsubfx/pkg-plist'
Restored 'audio/zynaddsubfx/Makefile'
Restored 'audio/zynaddsubfx/distinfo'
Restored 'audio/zynaddsubfx/pkg-descr'
D    graphics/py-gvgen/pkg-plist
U    graphics/py-gvgen/distinfo
U    graphics/py-gvgen/pkg-descr
U    graphics/py-gvgen/pkg-message
U    graphics/py-gvgen/Makefile
Updated to revision 355654.
-----------------------------------------

```

Notification of system tasks, which puts **CBSD** itself locally or on a remote node, to come to the email will not be, in addition, the director of tasks in this case keeps track of the end of the task, analyzes and removes the log refinement problem

For the formulation of the problem, use the command **cbsd task**. If the specified argument _node=XXXX_, task is created on a remote node, while, in the task list on the local machine you can track its status.

Copyright © 2013—2024 CBSD Team.

