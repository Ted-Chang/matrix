#!/bin/sh

usage="usage: update_img.sh [option]
    -h print this help message
    -f update floppy disk
    -d update hard disk
    -a update both floppy and hard disk"

tool_path="tools/make_initrd"

update_prepare()
{
    # Check whether the tool exists and executable
    if [ ! -x "$tool_path" ]; then
	echo "$tool_path not exist or not executable"
	exit -1
    fi

    $tool_path bin/init init bin/crond crond bin/echo echo bin/unit_test unit_test \
	bin/ls ls bin/cat cat bin/clear clear bin/shutdown shutdown bin/mkdir mkdir \
	bin/date date bin/mount mount bin/umount umount bin/mknod mknod bin/dd dd \
	bin/initrd

}

# update the matrix kernel in the floopy image
update_flpy()
{
    result=`sudo losetup -a | grep loop0`
    if [ "$result" == "" ]; then
	sudo losetup /dev/loop0 ~/vm/matrix/matrix-flpy.img
    fi

    update_prepare

    echo "Copying files to floppy disk"

    # Mount the root partition onto /mnt
    sudo mount /dev/loop0 /mnt/matrix/

    sudo rm -f /mnt/matrix/matrix
    sudo rm -f /mnt/matrix/initrd
    sudo cp bin/matrix /mnt/matrix/
    sudo cp bin/initrd /mnt/matrix/

    sync

    # Unmount the root partition
    sudo umount /mnt/matrix
}

# update the matrix kernel in the hard disk image
update_hd()
{
    if [ ! -e "/dev/mapper/hda" ]; then
        # Loop mount the disk image
	sudo losetup /dev/loop1 ~/vm/matrix/matrix-hd.img
	# Create device mapper node for root device
	echo '0 101808 linear 7:1 0' | sudo dmsetup create hda
    fi

    update_prepare

    echo "Copying files to hard disk"

    # Mount the root partition onto /mnt
    sudo mount /dev/mapper/hda /mnt/matrix
    
    sudo rm -f /mnt/matrix/matrix
    sudo rm -f /mnt/matrix/initrd
    # Copy the files to the disk image
    sudo cp bin/matrix /mnt/matrix/
    sudo cp bin/initrd /mnt/matrix/

    # Sleep for 1 seconds as devmapper will be busy if we unmount it now
    sleep 1s

    # Sync data to the image file
    sync

    # Umount the root partition
    sudo umount /mnt/matrix
}

case "$1" in
    "-h")
	echo "$usage"
	;;
    "-f")
	update_flpy
	;;
    "-d")
	update_hd
	;;
    "-a")
	update_flpy
	update_hd
	;;
    *)
	# If no option provided, just update the hard disk
	update_hd
	;;
esac

