#!/bin/sh

#
# To add new binary into initrd, just append the path to command in 
# update_prepare() function
#

usage="usage: update_img.sh [option]
    -h print this help message
    -f update floppy disk
    -d update hard disk
    -a update both floppy and hard disk"

tool_path="tools/make_initrd"
image_dir="images"
mnt_point="images/mnt"

# Exit with an error message
die()
{
    echo $@
    exit -1
}

check_env()
{
    # Check whether the tool exists and executable
    if [ ! -x "$tool_path" ]; then
	die "$tool_path not exist or not executable"
    fi

    # Check whether image directory exists
    if [ ! -d "$image_dir" ]; then
	die "$image_dir not found"
    fi

    # Check whether mount point exists
    if [ ! -d "$mnt_point" ]; then
	die "$mnt_point not found"
    fi
}

update_prepare()
{
    echo "Preparing init ramdisk"

    $tool_path bin/init init bin/crond crond bin/echo echo bin/unit_test unit_test \
	bin/ls ls bin/cat cat bin/clear clear bin/shutdown shutdown bin/mkdir mkdir \
	bin/date date bin/mount mount bin/umount umount bin/mknod mknod bin/dd dd \
	bin/lsmod lsmod bin/initrd

}

update_files()
{
    echo "Updating files"

    sudo rm -f "$mnt_point/matrix"
    sudo rm -f "$mnt_point/initrd"
    sudo cp bin/matrix "$mnt_point"
    sudo cp bin/initrd "$mnt_point"

    sync
}

# update the matrix kernel in the floopy image
update_flpy()
{
    flpy_img="$image_dir/matrix-flpy.img"

    if [ ! -e "$flpy_img" ]; then
	die "$flpy_img not found"
    fi

    update_prepare

    sudo losetup /dev/loop0 "$flpy_img"

    sleep 1s

    # Mount the root partition onto $mnt_point
    sudo mount /dev/loop0 "$mnt_point"

    update_files

    # Unmount the root partition
    sudo umount "$mnt_point"
}

# update the matrix kernel in the hard disk image
update_hd()
{
    hd_img="$image_dir/matrix-hd.img"

    if [ ! -e "$hd_img" ]; then
	die "$hd_img not found"
    fi

    update_prepare

    sudo losetup /dev/loop1 "$hd_img"

    sleep 1s

    echo "Mounting hard disk image file"

    # Mount the root partition onto /mnt
    sudo mount /dev/loop1 "$mnt_point"
    
    update_files

    # Umount the root partition
    sudo umount "$mnt_point"

    # Detach loopback device
    sudo losetup -d /dev/loop1
}

case "$1" in
    "-h")
	echo "$usage"
	;;
    "-f")
	check_env
	update_flpy
	;;
    "-d")
	check_env
	update_hd
	;;
    "-a")
	check_env
	update_flpy
	update_hd
	;;
    *)
	# If no option provided, just update the hard disk
	update_hd
	;;
esac

