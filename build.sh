#!/bin/bash

if [ ! -d libdvbtee ] ; then
    git clone git://github.com/mkrufky/libdvbtee.git
fi

cd libdvbtee

export DVBTEE_ROOT="`pwd`"

if [ -n "$ARCH" -a -n "$CROSS_COMPILE" ] ; then
	# these shouldn't have to be set if CROSS_COMPILE is, but keeping to not break build
	export CC=${CROSS_COMPILE}gcc
	export CC=${CROSS_COMPILE}g++
	export CC=${CROSS_COMPILE}ld
	export CC=${CROSS_COMPILE}strip
else
    echo using default build environment...
fi

if [ "$HDHOMERUN_SUPPORT" == "1" ] ; then
    QMAKE_EXTRA=" "
else
    QMAKE_EXTRA=disallowhdhr
fi

if [ -e Makefile ]; then
    echo Makefile already exists...
else
    if [ -e .staticlib ]; then
        qmake -r CONFIG+="staticlib $QMAKE_EXTRA"
    else
        qmake -r CONFIG+="$QMAKE_EXTRA"
    fi
fi

mkdir -p usr/bin
mkdir -p usr/lib
mkdir -p usr/include

if [ -e .clean ]; then
    make clean -C libdvbpsi
    if [ $? != 0 ]; then
        echo "make clean (libdvbpsi) failed"
    	#exit 1 // dont exit
    fi

    make clean -C dvbtee
    if [ $? != 0 ]; then
        echo "make clean (dvbtee) failed"
    	exit 1
    fi
fi

if [ -e libdvbpsi ]; then
    cd libdvbpsi
else
    git clone git://github.com/mkrufky/libdvbpsi.git
    cd libdvbpsi
    patch -p2 < ../libdvbpsi-silence-TS-discontinuity-messages.patch
fi

if [ -e .configured ]; then
    git log -1
else
    ./bootstrap
    patch -p1 < ../dvbpsi-noexamples.patch
    if [ -n "$ARCH" -a -n "$CROSS_COMPILE" ] ; then
	./configure --prefix=${DVBTEE_ROOT}/usr/ --host=${ARCH} --target=${ARCH}
    else
	./configure --prefix=${DVBTEE_ROOT}/usr/  --disable-debug --disable-release
    fi
    touch ./.configured
fi
cd ..


make -C libdvbpsi
if [ $? != 0 ]; then
    echo "make (libdvbpsi) failed"
    exit 1
fi

make -C libdvbpsi install

make -C . -I${DVBTEE_ROOT}/usr/include/dvbpsi/
if [ $? != 0 ]; then
    echo "make (dvbtee) failed"
    exit 1
fi

make -C dvbtee install

# TODO: some Makefile target should do this...
cp -P libdvbtee/libdvbtee.so* libdvbtee_server/libdvbtee_server.so* usr/lib


