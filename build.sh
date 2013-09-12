#!/bin/bash

usage="usage: build.sh [option]
    -h print this help message
    -u build user space components
    -k build kernel space components
    -a build all components"

# Remove the object file 
rm -f bin/lib/*.o
rm -f bin/obji386/*.o
rm -f bin/sdk/*.o
rm -f uspace/obji386/*.o

build_kernel()
{
    pushd kernel
    make clean && make
    popd

    if [ ! -f "bin/matrix" ]; then
	echo "*** Build kernel failed ***"
	exit -1
    fi
}

build_uspace()
{
    pushd sdk
    make clean && make
    popd

    pushd uspace
    make clean && make
    popd
}

case "$1" in
    "-h")
	echo "$usage"
	;;
    "-u")
	build_uspace
	;;
    "-k")
	build_kernel
	;;
    "-a")
	build_kernel
	build_uspace
	;;
    *)
	echo "$usage"
	;;
esac
