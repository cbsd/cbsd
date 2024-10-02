# Fuse-Archive

`fuse-archive` is a program that serves an archive or compressed file (e.g.
`foo.tar`, `foo.tar.gz`, `foo.xz` or `foo.zip`) as a read-only
[FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace) file system.

It is similar to [`mount-zip`](https://github.com/google/mount-zip) and
[`fuse-zip`](https://bitbucket.org/agalanin/fuse-zip) but speaks a larger range
of archive or compressed file formats.

It is similar to [`archivemount`](https://github.com/cybernoid/archivemount) but
can be much faster (see the Performance section below) although it can only
mount read-only, not read-write.


## Build

    $ git clone https://github.com/google/fuse-archive.git
    $ cd fuse-archive
    $ make

On a Debian system, you may first need to install some dependencies:

    $ sudo apt install libarchive-dev libfuse-dev


## Performance

Create a single `.tar.gz` file that is 256 MiB decompressed and 255 KiB
compressed (the file just contains repeated 0x00 NUL bytes):

```
$ truncate --size=256M zeroes
$ tar cfz zeroes-256mib.tar.gz zeroes
```

Create a `mnt` directory:

```
$ mkdir mnt
```

`fuse-archive` timings:

```
$ time fuse-archive zeroes-256mib.tar.gz mnt
real    0m0.443s

$ dd if=mnt/zeroes of=/dev/null status=progress
524288+0 records in
524288+0 records out
268435456 bytes (268 MB, 256 MiB) copied, 0.836048 s, 321 MB/s

$ fusermount -u mnt
```

`archivemount` timings:

```
$ time archivemount zeroes-256mib.tar.gz mnt
real    0m0.581s

$ dd if=mnt/zeroes of=/dev/null status=progress
268288512 bytes (268 MB, 256 MiB) copied, 569 s, 471 kB/s
524288+0 records in
524288+0 records out
268435456 bytes (268 MB, 256 MiB) copied, 570.146 s, 471 kB/s

$ fusermount -u mnt
```

Here, `fuse-archive` takes about the same time to scan the archive, bind the
mountpoint and daemonize, but it is **~700× faster** (0.83s vs 570s) to copy out
the decompressed contents. This is because `fuse-archive` does not use
`archivemount`'s [quadratic complexity
algorithm](https://github.com/cybernoid/archivemount/issues/21).


## Disclaimer

This is not an official Google product. It is just code that happens to be owned
by Google.


---

Updated on May 2022.
