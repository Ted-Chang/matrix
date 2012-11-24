#!/bin/bash

rm -f bin/lib/*.o
rm -f bin/obji386/*.o
pushd kernel
make clean && make
popd
pushd sdk
make clean && make
popd
pushd userspace
make clean && make
popd
