# A few words about jail traffic counting

## command: fwcounters

**Description**:


Currently, the easiest way to count traffic - use functional of **ipfw count** on the JID of necessary environment.

Necessary conditions for the implementation of this:

- loaded module **ipfw.ko**;
- mounted in **CBSD** (for example via **cbsd initenv-tui**) ipfw\_enable parameters to 1;
- count the presence of the rules in ipfw **before** any other firewall rules.


**CBSD** starting and stopping the jails and removes automatically sets the rules for traffic jail, using a range of these rules setting **CBSD** (range by default fwcount\_st="99" - fwcount\_end="2000" )

In other words, if you want to count traffic and still have the ability to filter traffic, create filtering rules above 2000 and not taking rules from 99 to 2000.

Collect of traffic occurs 1 time per hour or when stopping the jail and stored in SQLite3 base, located in the system directory jail: _$workdir/jails-system/$jname/traffic/YYYY-MM.sqlite_, where YYYY, MM - year and month.

Example. Enjoying the traffic statistics for the jail kde4. meaning of the fields **outgoing** and **incoming** \- in bytes:

```
root@home:/ # sqlite3 /usr/jails/jails-system/kde4/traffic/2014-09.sqlite
SQLite version 3.8.6 2014-08-15 11:46:33
Enter ".help" for usage hints.

sqlite> .schema traffic
CREATE TABLE traffic (  dt TIMESTAMP DEFAULT (datetime('now','localtime')) UNIQUE PRIMARY KEY, incoming integer, outgoing integer  );

sqlite> select * from traffic order by dt desc limit 15;
2014-09-20 15:00:52|142704274|4958246
2014-09-20 14:00:51|163907026|25242205
2014-09-20 13:00:49|3894888|182821
2014-09-20 05:49:53|58329247|41769720
2014-09-20 05:00:23|24247445|3464945
2014-09-20 04:00:56|67749758|39433640
2014-09-20 02:28:36|11767628|438283
2014-09-20 02:00:57|115675943|10809029
2014-09-20 01:00:54|279397568|156221677
2014-09-20 00:00:51|223665101|6232876
2014-09-19 23:00:50|250122634|8619979
2014-09-19 22:00:49|221508227|6458218
2014-09-19 01:00:42|64715837|3443253
2014-09-19 00:00:38|109266525|8541659
2014-09-18 23:00:34|99594683|20380707
sqlite>

```

![](http://www.convectix.com/img/trafstat1.png)

![](http://www.convectix.com/img/trafstat2.png)

