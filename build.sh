#!/bin/bash

# To force disable HdHomeRun support, re-run this shell script with --disable-hdhr

if [ ! -d libdvbtee ] ; then
    git clone git://github.com/mkrufky/libdvbtee.git
fi

(cd libdvbtee; ./build-auto.sh --prefix=`pwd`/usr/ "$@" --enable-static; make install)
