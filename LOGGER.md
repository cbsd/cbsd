# cbsdlog

Status: design.

The goal is to log syslog messages from jails without running `syslogd` in each jail, and to log each jail's console.

One host daemon collects console output and syslog from CBSD jails if configured as syslog=1.
A dedicated `syslogd` writes the files into jails logs folder - one per jail by default.
`newsyslog` rotates them.

## Parts

1. `cbsdlogd`, the daemon. On a command from `cbsdlogctl` at jail start, it opens a datagram socket per jail, accepts
   the jail's stdout and stderr over IPC, and forwards every line to the dedicated `syslogd` with the jail name in
   the hostname field.
2. `cbsdlogctl`, the control tool. It starts a jail command with those descriptors attached, enables and disables
   logging for a jail, and shows the startup output.
3. A dedicated `syslogd` with its own config, socket, and pid file. An admin can edit the per-jail blocks.
4. A `newsyslog` snippet that rotates `${workdir}/var/log/jails/`.

## Daemon

`cbsdlogd` listens on `${workdir}/var/run/cbsdlogd.ctl` (mode `600`, root only). The control protocol is one text
command per line. `workdir` is `-w`, otherwise `$workdir` or `$cbsd_workdir`, otherwise `cbsd_workdir` from `/etc/rc.conf`.

```text
OPEN <jail> <path> <mode>
CLOSE <jail>
CONSOLE <jail>
FOLLOW <jail>
```

Every command is answered with `OK` or `ERR <text>`.

`OPEN` binds a `SOCK_DGRAM` socket at `path` and remembers that every datagram on it belongs to `jail`. A jail is
opened twice: mode `666` for the ordinary socket and mode `600` for the privileged one. `CLOSE` closes both and
unlinks only sockets this daemon created. The jail name may contain letters, digits, `.`, `_`, and `-`, so it is safe
as a file name. The socket path is absolute. For a jail it is `${data}/var/run/log` and
`${data}/var/run/logpriv`: that directory is the jail's `/var/run`, so software inside the jail finds the usual
`/var/run/log` and `/var/run/logpriv`. It is never the host's `/var/run/log`.

`CONSOLE` carries the read ends of the jail command's stdout and stderr as `SCM_RIGHTS` file descriptors. Stdout is
forwarded as facility `console`, priority `info`. Stderr is facility `console`, priority `err`. The daemon splits each
byte stream into lines. The jail name is the one named in the command.

A datagram is parsed only far enough to find a spoofed hostname. It must start with `<pri>` and an RFC 3164 or
RFC 5424 header; anything else is dropped. The daemon does not invent a timestamp for the log socket. After `<pri>`
and the timestamp, the first word is a hostname when a space comes before any `:`. That word is discarded. When `:`
comes first, the message has no hostname, which is what libc `syslog()` sends. RFC 5424 uses the hostname field after
the version and the timestamp. In every case the hostname written onward is the jail name from `OPEN`. A newline
inside one datagram becomes a space, so one datagram stays one line. The daemon then sends that datagram to the
dedicated `syslogd`. Console lines are not syslog datagrams: for those the daemon writes an RFC 3164 header itself.

The daemon keeps each jail's console in one 32 KiB buffer. A line is a tag byte, the text, and `\n`. The tag is
`0x01` for stdout and `0x02` for stderr. `start` is the oldest byte and `end` is the next free byte. When `end` is
before `start`, the bytes wrap: `start` to the end of the buffer, then the beginning up to `end`. A new line drops
whole lines from `start` until it fits. `cbsdlogctl console` reads that buffer and writes each line to its own stdout
or stderr. The tag is not printed and is not part of the syslog message. The buffer is not the log file: an admin
may later drop `console.info` from the jail's syslog block, and stdout would then be absent from
`${workdir}/var/log/jails/<jail>.log`.

## Control tool

`cbsdlogctl exec <jail> -- <command>` is the jail's `exec_start`. It creates two pipes, sends the read ends with
`CONSOLE`, and waits until the daemon has stored them. It then `dup2`s the write ends onto stdout and stderr and
`exec`s the command. The ctl process does not stay around. The command keeps its own stdin. Anything that inherits
those descriptors, including `/etc/rc` or a long-running entrypoint, is the console stream.

`jstart` inserts that command between `nice` and `jail_env`: `daemonize` still starts the jail, and `cbsdlogctl`
replaces stdout and stderr before `jail_env` runs. `exec.start` inherits them, so `/etc/rc` writes the pipes from
inside the jail. When `syslog` is off the inserted command is empty. A vnet or zfs-attach jail does not run `rc`
inside `jail -c`; there the same prefix is placed between `cpuset` and `jexec`.

`cbsdlogctl console <jail>` sends `FOLLOW`. After `OK` it prints the buffered console lines and then follows new
ones. Stdout lines go to its stdout and stderr lines go to its stderr. `--color auto` (the default) paints stdout
green and stderr red when that stream is a terminal. `yes` always paints, `no` never does. It exits when no process still has that
jail's console open: both stdout and stderr have reached EOF. That is
the output of `/etc/rc` or the entrypoint while the process is writing it. `-t` / `--timeout` seconds stops the
follow even if the command is still running. An OCI jail's entrypoint does not exit, so a non-quiet start shows
`console -t 2`.

`console --wait [seconds]` sends `FOLLOW <jail> wait`. If the console is not open, the client stays until the next
session opens and then closes. `seconds` limits only that open wait; omitted or `0` waits without a limit. Once the
console is open there is no open timeout, and `-t` still limits the follow. The daemon marks the open with a single
NUL, which `cbsdlogctl` does not print. A non-quiet `jstart` starts `--wait` with the parallel timeout before
`jail -c`, so `/etc/rc` is printed while it runs. A vnet or zfs-attach jail runs `rc` from `jexec`, so `jstart`
follows that session with a second `--wait` beside `jexec`. If the console does not open in time, `--wait` exits
`2` and `jstart` falls back to `console` without `--wait`, which replays the buffer.

A jail property `syslog` (default `0`) turns this on. `jcreate` runs `cbsdlogctl create`. `jset` runs it when `syslog` changes to `1`. If `${data}/etc/rc.conf` exists, both also set `syslogd_enable="NO"` there. The jail's own `syslogd` would otherwise unlink `/var/run/log` and `/var/run/logpriv` and bind them itself. Linux and empty jails have no such file and are left alone. `jstart` runs `cbsdlogctl enable` with the socket directory `${data}/var/run`, then puts `cbsdlogctl exec` in front of `jail_env` or `jexec`. `jstop` runs `cbsdlogctl disable`. `jdestroy` runs `cbsdlogctl destroy`.

`enable` asks the daemon to bind the jail's sockets. `disable` closes them and drops the console buffer.

`create <jail>` writes `${workdir}/etc/cbsdlog.d/<jail>.conf` once, creates the log file, and signals the
dedicated `syslogd`. An existing `.conf` is left as the admin wrote it. `destroy <jail>` removes that `.conf` and
signals `syslogd`. The log file stays. Neither command talks to `cbsdlogd`.

`etc/cbsdlog.conf` in the CBSD tree is a template. `initenv` copies it to `${workdir}/etc/cbsdlog.conf` once, replacing
`%%workdir%%`, and does not overwrite a file that is already there. That copy is what `syslogd -f` reads. It includes
`${workdir}/etc/cbsdlog.d`. `syslogd` reads every `*.conf` in that directory. A new `+` line replaces
the hostname filter, so blocks do not need `+*` between them.

That file also has one rule for the program `cbsdlog-keep`, which is never used. FreeBSD `syslogd` aborts when the
parsed config has no actions: the casper helper returns success without an `nvl_conf` nvlist, and
`nvlist_take_nvlist()` raises `SIGABRT`. An empty `cbsdlog.d` (no jail created yet, or the last `destroy`) is that
case. The placeholder rule keeps the process up. Included files reset the program filter, so it does not apply to
jail blocks.

## Recreating a removed socket

`OPEN` binds `${data}/var/run/log` and `${data}/var/run/logpriv` before the jail's `/etc/rc` runs. Inside the jail
those names are `/var/run/log` and `/var/run/logpriv`. `cleanvar` keeps sockets with those names. With
`syslogd_enable="NO"` the jail's `syslogd` does not unlink them either. If the name still disappears, the
daemon's file descriptor stays open, but `syslog()` looks up the path and does not find it.

After each successful bind, `cbsdlogd` watches that descriptor with `kqueue` (`EVFILT_VNODE`, `NOTE_DELETE` and
`NOTE_REVOKE`). When the name disappears, the daemon closes the old descriptor and binds the same path again at the
same mode. The name is back before services that open syslog connect. A process
that had already connected keeps the old inode until `send` fails and libc opens the path again. A failed rebind is
reported to the host `syslogd` as `cbsdlogd`.

## Boot

`rc.d/cbsdlogd` is the system service. `initenv` links it to `/usr/local/etc/rc.d/cbsdlogd` when that path is missing.
Boot runs it only when `cbsdlogd_enable="YES"` is set in `/etc/rc.conf`. It is ordered `BEFORE: cbsdd`, so the logger
is up before `jails-astart`.

The script starts two processes. First the dedicated `syslogd`, then `cbsdlogd`. `cbsdlogd` daemonizes itself and
writes `${workdir}/var/run/cbsdlogd.pid`. Stop sends `SIGTERM` to both pid files and waits until
those processes exit. A dedicated `syslogd` left without its pid file is found by its `-f` path and stopped
the same way. Starting the service again does nothing when that process is already alive.

```text
syslogd -ss -H \
  -f ${workdir}/etc/cbsdlog.conf \
  -p ${workdir}/var/run/jail-syslog.sock \
  -P ${workdir}/var/run/jail-syslog.pid \
  -S ${workdir}/var/run/jail-syslog.priv
```

`-p`, `-P`, and `-S` keep this `syslogd` off the host's `/var/run/log`, `/var/run/syslog.pid`, and `/var/run/logpriv`.
`-ss` opens no network socket. `-H` keeps the jail name carried in the datagram.

## Dedicated syslogd

This process is not the host's `syslogd`. It is started with `-H`, its own `-p` socket, its own `-f` config, and its
own `-P` pid file. `-ss` keeps it off the network. Opening `/dev/klog` fails because the system daemon already holds
it; the process continues without kernel messages.

`-H` is required. For a local socket, `syslogd` otherwise throws away the hostname inside the datagram and stamps the
host's own name. `+jail1` would then match nothing.

The dedicated `syslogd` is started with `-f ${workdir}/etc/cbsdlog.conf`. That file includes
`${workdir}/etc/cbsdlog.d`, and each jail is one `<jail>.conf`:

```text
+jail1
*.*						${workdir}/var/log/jails/jail1.log

+jail2
*.*						${workdir}/var/log/jails/jail2.log
```

`!jail1` is the wrong filter: `!` matches the program tag (`sshd`, `xpra`), not the jail. The jail name is the
hostname field.

The default selector is `*.*`. `*.>=notice` does not write stdout, because `notice` is more severe than `info` and
stdout is `console.info`. Stderr (`err`) would still pass. An admin who keeps a `notice` threshold adds a separate
`console.info` line to the same file.

Each `<jail>.conf` is created once and is not overwritten. The admin can split a jail across files or raise the
priority. `create` only adds a missing file and sends `SIGHUP`. `destroy` deletes the file and sends `SIGHUP`.

## Rotation

`/usr/local/etc/newsyslog.conf.d/` holds one glob. The pid file belongs to the dedicated `syslogd`, because that
process holds the log files open. A signal to `cbsdlogd` or to the system `syslogd` would keep writing into the
rotated inode.

```text
${workdir}/var/log/jails/*.log  640  14  10240  @T00  JG  ${workdir}/var/run/jail-syslog.pid
```

`G` makes the path a pattern, so a file an admin adds under `${workdir}/var/log/jails/` is rotated too. The threshold is 10 MB
or midnight, whichever comes first. Fourteen compressed archives are kept. `newsyslog` already runs hourly from cron;
a file may grow for about an hour past the size limit.
