#!/bin/sh

# update the matrix kernel in the floopy image
#sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
sudo mount /dev/loop0 /mnt
sudo cp matrix /mnt
sudo umount /dev/loop0