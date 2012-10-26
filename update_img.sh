#!/bin/sh

# update the matrix kernel in the floopy image
#sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
tools/makeinitrd bin/init init bin/crond crond bin/initrd bin/echo echo
sudo mount /dev/loop0 /media/matrix
sudo cp bin/matrix /media/matrix
sudo cp bin/initrd /media/matrix
sudo umount /dev/loop0