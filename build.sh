#!/bin/sh

if [ ! -d libdvbtee ] ; then
	git clone git://github.com/mkrufky/libdvbtee.git
	patch -p0 -i libdvbtee.patch
fi
cd libdvbtee
touch .x86
./build.sh
cp -P libdvbtee/libdvbtee.so* libdvbtee_server/libdvbtee_server* usr/lib
