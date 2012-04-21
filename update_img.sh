#!/bin/sh

# update the matrix kernel in the floopy image
#sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
sudo mount /dev/loop0 /mnt
objcopy --only-keep-debug bin/matrix bin/matrix.sym
objcopy --strip-debug matrix
sudo cp bin/matrix /mnt
sudo umount /dev/loop0