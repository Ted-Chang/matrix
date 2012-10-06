#!/bin/sh

# update the matrix kernel in the floopy image
#sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
sudo mount /dev/loop0 /media
sudo cp bin/matrix /media
sudo cp initrd /media
sudo umount /dev/loop0