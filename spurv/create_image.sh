#!/bin/bash

ROOTDIR="`dirname "$0"`/../../../.."
cd $ROOTDIR

echo "Copying kernel to rootfs"
cp linux/arch/arm/boot/zImage rootfs/boot/.

echo "Adding RDU2 Support"
cp linux/arch/arm/boot/dts/imx6qp-zii-rdu2.dtb rootfs/boot/.

echo "Adding Nitrogen6QP Max Support"
cp linux/arch/arm/boot/dts/imx6qp-nitrogen6_max.dtb rootfs/boot/.
mkimage -A arm -O linux -T script -C none -n boot.scr -d uboot_nitrogen6qp-max.scr rootfs/boot/boot.scr &> /dev/null

echo "Copying Android runtime to rootfs"
cp android/device/freedesktop/spurv/launch-container.sh rootfs/home/aosp/.
cp android/device/freedesktop/spurv/run.sh rootfs/home/aosp/.
cp android/device/freedesktop/spurv/run_demo.sh rootfs/home/aosp/.
cp android/out/target/product/spurv/system.img rootfs/home/aosp/aosp.img
cp android/out/target/product/spurv/vendor.img rootfs/home/aosp/.
cp android/out/target/product/spurv/userdata.img rootfs/home/aosp/.


sudo mkdir /mnt/sdcard &> /dev/null
sudo umount /dev/mmcblk0p1 &> /dev/null
sudo mount /dev/mmcblk0p1 /mnt/sdcard

echo "Copying rootfs to sdcard"
sudo cp -rfa rootfs/* /mnt/sdcard/.
echo "Syncing.."
sync
echo "Done"
echo ""

echo "Creating image"
IMG_NAME="aosp_demo_$(date +%Y-%m-%d).img"
dd if=/dev/zero of=$IMG_NAME bs=1024M count=8

echo "Creating filesystem"
TMP_DIR=$(mktemp -d /tmp/loop_XXXXX)
LOOP_DEV=$(sudo losetup -f $IMG_NAME --show)

sudo parted --script $LOOP_DEV \
            mktable msdos \
            mkpart primary ext4 4096s 100%

## Remount to get an updated MBR
sudo losetup -d $LOOP_DEV
LOOP_DEV=$(sudo losetup -f $IMG_NAME --show)

## Format partition #1 to EXT4
sudo mkfs.ext4 ${LOOP_DEV}p1

echo "Copying rootfs to image"
sudo mount -o rw ${LOOP_DEV}p1 $TMP_DIR
sudo cp -rfa rootfs/* $TMP_DIR
sudo umount $TMP_DIR

echo "Compressing image"
pbzip2 $IMG_NAME -f > ${IMG_NAME}.bz2