PREFIX?=/usr/local

all:

install:
	mkdir -p ${PREFIX}/cbsd
	cp -Rp * ${PREFIX}/cbsd/
