#!/bin/bash

pushd kernel
make clean && make
popd
