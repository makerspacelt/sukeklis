#!/bin/bash

me_was_here=$(pwd);
rm -fr /tmp/linux_source
mkdir /tmp/linux_source
cd /tmp/linux_source
wget -O linux.tar.gz https://github.com/raspberrypi/linux/tarball/rpi-patches
tar xzf linux.tar.gz
mv raspberrypi-linux-* linux
cd linux
zcat /proc/config.gz  > .config
sed -i 's/EXTRAVERSION =.*/EXTRAVERSION = +/' Makefile
wget -O Module.symvers https://raw.github.com/raspberrypi/firmware/master/extra/Module.symvers
make oldconfig
make modules_prepare
cd $me_was_here

#sudo rm -fr /lib/modules/$(uname -r)/build
#sudo mv /tmp/linux_source/linux /lib/modules/$(uname -r)/build
echo "DONE"

