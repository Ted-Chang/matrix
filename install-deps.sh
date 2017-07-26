#!/bin/bash -e
#
# matrix - operating system expirement project
#
# Copyright (C) 2017 Ted Zhang
#
# Author: Ted Zhang <ted.g.zhang@live.com>
#
source /etc/os-release
case $ID in
    centos|fedora|rhel)
	echo "Using yum to install dependencies"
	$SUDO yum install -y redhat-lsb-core
	# Install nasm gcc and make
	yum install -y nasm gcc make
	;;
    debian|ubuntu|devuan|opensuse|suse|sles)
	echo "Untested platform, please install nasm, gcc, make manually."
	;;
    *)
	echo "$ID is unknown, dependencies will have to be installed manually."
	;;
esac
		
