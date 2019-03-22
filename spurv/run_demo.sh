#!/bin/bash

ROOTDIR=`dirname "$0"`
cd $ROOTDIR

echo 100 >  /sys/bus/platform/drivers/rave_sp_backlight/21f0000.serial\:rave-sp\:backlight/backlight/21f0000.serial\:rave-sp\:backlight/brightness

# Mount AOSP system.img
umount aosp
sync
mkdir -p aosp
mount -o loop,rw aosp.img aosp

# Enforce probing order so that:
# imx-drm -> /dev/dri/card0
# etnaviv -> /dev/dri/card1
echo etnaviv > /sys/bus/platform/drivers/etnaviv/unbind
echo display-subsystem > /sys/bus/platform/drivers/imx-drm/unbind
echo display-subsystem > /sys/bus/platform/drivers/imx-drm/bind
echo etnaviv > /sys/bus/platform/drivers/etnaviv/bind

bash aosp/launch-container.sh --width 1280 --angry &

sleep 3
export XDG_RUNTIME_DIR=/run/user/1000
/usr/lib/weston/weston-terminal &
/usr/lib/weston/weston-simple-egl &
glmark2-es2-wayland --run-forever &

# Sleep forever
tail -f /dev/null
