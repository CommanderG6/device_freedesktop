## Android

    mkdir aosp; cd aosp

Install repo as per https://source.android.com/setup/build/downloading

    repo init -u https://android.googlesource.com/platform/manifest -b android-9.0.0_r10
    git clone https://gitlab.collabora.com/zodiac/android_manifest.git .repo/local_manifests/
    repo sync -j15
    . build/envsetup.sh
    lunch spurv-eng
    make -j12

## Kernel

    git clone https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux
    cd linux
    git reset --hard v4.20
    sh ../aosp/device/freedesktop/spurv/build-kernel.sh

The kernel image is at arch/arm/boot/zImage and the DT at arch/arm/boot/dts/imx6qp-zii-rdu2.dtb

## Root filesystem

    sudo apt install debootstrap qemu-user-static
    sudo debootstrap --include=systemd,weston,systemd-container,udev,sudo,openssh-server,iputils-ping,pulseaudio --arch armhf --variant minbase testing rootfs http://deb.debian.org/debian/
    sudo chroot rootfs adduser aosp --ingroup sudo
    
#### Prepare sdcard

    mkfs.ext4 /dev/mmcblk0
    sudo mkdir /mnt/sdcard
    sudo mount /dev/mmcblk0 /mnt/sdcard

#### Copy files to sdcard

    sudo cp linux/arch/arm/boot/zImage linux/arch/arm/boot/dts/imx6qp-zii-rdu2.dtb rootfs/boot/.
    cp aosp/device/freedesktop/spurv/run.sh rootfs/home/aosp/.
    cp aosp/out/target/product/spurv/system.img rootfs/home/aosp/aosp.img
    cp aosp/out/target/product/spurv/vendor.img rootfs/home/aosp/.
    cp aosp/out/target/product/spurv/userdata.img rootfs/home/aosp/.
    sudo cp -rfa rootfs/* /mnt/sdcard/.


## Boot SD card

#### In barebox's console

    detect mmc1
    global.linux.bootargs.base="console=ttymxc0,115200 console=tty0 enforcing=0 ip=dhcp rw rootwait root=/dev/mmcblk0 log_buf_len=16M cma=512M vmalloc=512M" 
    global.bootm.oftree=/mnt/mmc1/boot/imx6qp-zii-rdu2.dtb
    global.bootm.image=/mnt/mmc1/boot/zImage
    global.bootm.initrd=
    bootm

#### Run!

Log in as aosp/aosp

    sh run.sh