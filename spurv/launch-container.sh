rm -f /home/tomeu/aosp/vendor
mkdir -p /home/tomeu/aosp/vendor
mount -o loop,rw /home/tomeu/vendor.img /home/tomeu/aosp/vendor
mount -o loop,rw /home/tomeu/userdata.img /home/tomeu/aosp/data

systemd-nspawn --boot \
               --tmpfs /metadata \
               --bind /dev/pts \
               --bind /dev/ptmx \
               --bind /dev/null \
               --bind /dev/dri/renderD128 \
               --bind /dev/kmsg \
               --bind /dev/kmsg:/dev/kmsg_debug \
               --bind /dev/binder \
               --bind /dev/hwbinder \
               --bind /dev/vndbinder \
               --bind /dev/ashmem \
               --bind /dev/urandom \
               --bind /dev/random \
               --bind /dev/urandom:/dev/hw_random \
               --bind /sys \
               --bind /run/user/1000/wayland-0 \
               --network-zone=aosp \
               --directory `dirname "$0"`
