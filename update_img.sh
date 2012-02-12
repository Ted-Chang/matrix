#!/bin/sh

# update the matrix kernel in the floopy image
sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
sudo mount /dev/loop0 /mnt
sudo cp src/matrix /mnt/matrix
sudo umount /dev/loop0