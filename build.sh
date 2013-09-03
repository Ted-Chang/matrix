#!/bin/bash

# Remove the object file 
rm -f bin/lib/*.o
rm -f bin/obji386/*.o
rm -f bin/sdk/*.o

pushd kernel
make clean && make
popd

if [ ! -f "bin/matrix" ]; then
    echo "*** Build kernel failed ***"
    exit -1
fi

pushd sdk
make clean && make
popd

pushd uspace
make clean && make
popd
