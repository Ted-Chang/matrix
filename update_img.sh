#!/bin/sh

# update the matrix kernel in the floopy image
#sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
tools/make_initrd bin/init init bin/crond crond bin/echo echo bin/unit_test unit_test bin/initrd
sudo mount /dev/loop0 /mnt/matrix
sudo cp bin/matrix /mnt/matrix
sudo cp bin/initrd /mnt/matrix
sudo umount /dev/loop0