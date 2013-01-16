#!/bin/sh

if [ ! -d libdvbtee ] ; then
	git clone git://github.com/mkrufky/libdvbtee.git
fi
cd libdvbtee
touch .x86
./build.sh
cp -P libdvbtee/libdvbtee.so* libdvbtee_server/libdvbtee_server* usr/lib
