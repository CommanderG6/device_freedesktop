#!/bin/bash -x

APP=fishnoodle.canabalt
WIDTH=1920
HEIGHT=1080

for arg in "$@"; do
  case "$1" in
    "--angry") APP=com.rovio.angrybirds ;;
    "--width") WIDTH="$2" HEIGHT=$(bc <<< "$WIDTH * 0.5625 / 1") ;;
  esac
  shift
done

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

chmod a+rw /run/user/1000/

XDG_RUNTIME_DIR=/run/user/1000/ weston --tty=7 &

rm -rf /run/aosp/pulse
mkdir -p /run/aosp
chmod a+rw /run/aosp
PULSE_RUNTIME_PATH=/run/aosp/pulse pulseaudio --system &

sleep 4 # Wait for /run/user/1000/wayland-0 to be created
chmod a+rw /run/user/1000/wayland-0
chmod a+rw /run/aosp/pulse/native

dd if=$ROOTDIR/selinux-policy.bin of=/sys/fs/selinux/load bs=20M
setenforce 0

echo spurv.application=$APP >> $ROOTDIR/default.prop
echo spurv.display_width=$WIDTH >> $ROOTDIR/default.prop
echo spurv.display_height=$HEIGHT >> $ROOTDIR/default.prop

systemd-nspawn --boot \
               --tmpfs /metadata \
               --bind /dev/pts \
               --bind /dev/ptmx \
               --bind /dev/null \
               --bind /dev/dri/renderD128 \
               --bind /dev/dri/card0 \
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
               --bind /run/user/1000/wayland-0 \
               --bind /run/aosp/pulse/native \
               --register no \
               --resolv-conf copy-host \
               --link-journal host \
               --directory $ROOTDIR

# --network-zone=aosp

killall pulseaudio
killall weston

