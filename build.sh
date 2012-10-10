#!/bin/bash

pushd kernel
make clean && make
popd
pushd sdk
make clean && make
popd
pushd userspace
make clean && make
popd
