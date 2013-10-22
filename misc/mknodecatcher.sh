#!/bin/sh
cc nodecatcher.c sqlhelper.c gentools.c -o nodecatcher -lsqlite3 -L/usr/local/lib -I/usr/local/include

