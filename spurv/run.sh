echo 100 >  /sys/bus/platform/drivers/rave_sp_backlight/21f0000.serial\:rave-sp\:backlight/backlight/21f0000.serial\:rave-sp\:backlight/brightness
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

bash aosp/launch-container.sh
umount aosp/vendor
umount aosp/data
umount aosp
