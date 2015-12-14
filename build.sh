#!/bin/bash

if [ ! -d libdvbtee ] ; then
    git clone git://github.com/mkrufky/libdvbtee.git
fi

(cd libdvbtee; ./build-auto.sh --prefix=`pwd`/usr/ --enable-static; make install)
# To force disable HdHomeRun support, disable the above & enable the following:
# (cd libdvbtee; ./build-auto.sh --prefix=`pwd`/usr/ --enable-static --disable-hdhr; make install)
