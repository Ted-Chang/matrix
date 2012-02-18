#!bin/sh

# run bochs emulator in debugger mode
sudo losetup /dev/loop0 matrix.img
sudo bochs -q -f bochsrc.txt
sudo losetup -d /dev/loop0