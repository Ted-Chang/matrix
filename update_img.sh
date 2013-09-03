#!/bin/sh

tool_path="tools/make_initrd"

# update the matrix kernel in the floopy image
result=`sudo losetup -a | grep loop0`
if [ "$result" == "" ]; then
    sudo losetup /dev/loop0 ~/vm/matrix/matrix.img
fi

if [ ! -x "$tool_path" ]; then
    echo "$tool_path not exist or not executable"
    exit -1
fi

$tool_path bin/init init bin/crond crond bin/echo echo bin/unit_test unit_test \
bin/ls ls bin/cat cat bin/clear clear bin/shutdown shutdown bin/mkdir mkdir \
bin/date date bin/mount mount bin/umount umount bin/crond crond bin/initrd

sudo mount /dev/loop0 /mnt/matrix
sudo cp bin/matrix /mnt/matrix
sudo cp bin/initrd /mnt/matrix
sudo umount /dev/loop0