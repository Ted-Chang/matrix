#!/bin/bash

rm -f bin/lib/*.o
rm -f bin/obji386/*.o
pushd kernel
make clean && make
popd
pushd ddk
make clean && make
popd
pushd sdk
make clean && make
popd
pushd uspace
make clean && make
popd
