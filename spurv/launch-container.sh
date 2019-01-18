#!/bin/bash -x

ROOTDIR=`dirname "$0"`
chmod a+rw /dev/*binder /dev/ashmem
chmod a+x /sys/kernel/debug/
chmod a+x /sys/kernel/debug/tracing/
chmod a+rw /sys/kernel/debug/tracing/trace_marker
chmod a+rw /sys/kernel/debug/sync/sw_sync
chmod a+rw /dev/hwrng
chmod a+rw /dev/dri/*
chmod a+rw /dev/input/*
chmod a+rw /sys/power/wake_lock
chmod a+rw /sys/power/wake_unlock
chmod a+rw /dev/kmsg
mkdir -p $ROOTDIR/vendor
mount -o loop,rw vendor.img $ROOTDIR/vendor
mount -o loop,rw userdata.img $ROOTDIR/data

[ ! -d /run/user/1000 ] && mkdir -p /run/user/1000
[ ! -d /var/log/journal ] && mkdir -p /var/log/journal
[ ! -d /dev/socket ] && mkdir /dev/socket
XDG_RUNTIME_DIR=/run/user/1000/ weston --tty=7 &

sleep 4 # Wait for /run/user/1000/wayland-0 to be created
chmod a+rw /run/user/1000/wayland-0
ls -l /run/user/1000/wayland-0

dd if=$ROOTDIR/selinux-policy.bin of=/sys/fs/selinux/load bs=20M
setenforce 0

systemd-nspawn --boot \
               --tmpfs /metadata \
               --bind /dev/pts \
               --bind /dev/ptmx \
               --bind /dev/null \
               --bind /dev/dri/renderD128 \
               --bind /dev/dri/card1 \
               --bind /dev/kmsg \
               --bind /dev/kmsg:/dev/kmsg_debug \
               --bind /dev/binder \
               --bind /dev/hwbinder \
               --bind /dev/vndbinder \
               --bind /dev/ashmem \
               --bind /dev/urandom \
               --bind /dev/random \
               --bind /dev/socket \
               --bind /sys/kernel/debug/sync/sw_sync \
               --bind /dev/urandom:/dev/hw_random \
               --bind /sys \
               --bind /run/user/1000/wayland-0 --register no \
	       --resolv-conf copy-host \
	       --link-journal host \
               --directory $ROOTDIR

# --network-zone=aosp

killall weston

