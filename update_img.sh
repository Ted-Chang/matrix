#!/bin/sh

#
# To add new binary into initrd, you need to update the command in
# make_initrd() function
#

usage="usage: update_img.sh [option]
    -h print this help message
    -f update floppy disk
    -d update hard disk
    -a update both floppy and hard disk"

tool_path="tools/make_initrd"
image_dir="images"
mnt_point="images/mnt"
loop_dev=""

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

make_initrd()
{
    echo "Preparing init ramdisk"

    $tool_path bin/init init bin/crond crond bin/echo echo bin/unit_test unit_test \
	bin/ls ls bin/cat cat bin/clear clear bin/shutdown shutdown bin/mkdir mkdir \
	bin/date date bin/mount mount bin/umount umount bin/mknod mknod bin/dd dd \
	bin/lsmod lsmod bin/initrd

}

update_kernel_and_initrd()
{
    echo "Updating kernel and initrd"

    sudo rm -f "$mnt_point/matrix"
    sudo rm -f "$mnt_point/initrd"
    sudo cp bin/matrix "$mnt_point"
    sudo cp bin/initrd "$mnt_point"

    sync
}

find_loopdev()
{
    res=$(sudo losetup -l | grep "$1");
    if [ -z "$res" ]; then
        loop_dev="";
    else
        loop_dev=$(echo $res | awk '{print $1}')
	echo "Found loop device: $loop_dev"
    fi
}

setup_loopdev()
{
    loop_dev=$(sudo losetup --show --find "$1")
    echo "Set up loop device: $loop_dev"
}

update_image()
{
    img="$image_dir/$1"

    if [ ! -e "$img" ]; then
	die "$img not found"
    fi

    make_initrd

    # First try whether we can find the loop device
    find_loopdev "$img"
    if [ -z "$loop_dev" ]; then
        setup_loopdev "$img"
    fi

    # Mount the root partition onto $mnt_point
    echo "Mounting $loop_dev to $mnt_point"
    sudo mount $loop_dev "$mnt_point"

    update_kernel_and_initrd

    # Unmount the root partition
    echo "Umounting $loop_dev"
    sudo umount "$loop_dev"

    # Delete loopback device
    echo "Deleting $loop_dev"
    sudo losetup -d "$loop_dev"

    echo "Done."
}

case "$1" in
    "-h")
	echo "$usage"
	;;
    "-f")
	check_env
	update_image "matrix-flpy.img"
	;;
    "-d")
	check_env
	update_image "matrix-hd.img"
	;;
    "-a")
	check_env
	update_image "matrix-flpy.img"
	update_image "matrix-hd.img"
	;;
    *)
	# If no option provided, just update the hard disk image
	update_image "matrix-hd.img"
	;;
esac

