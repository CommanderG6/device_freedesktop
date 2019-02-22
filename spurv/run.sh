echo 100 >  /sys/bus/platform/drivers/rave_sp_backlight/21f0000.serial\:rave-sp\:backlight/backlight/21f0000.serial\:rave-sp\:backlight/brightness
umount aosp
sync
mkdir -p aosp
mount -o loop,rw aosp.img aosp
bash aosp/launch-container.sh
umount aosp/vendor
umount aosp/data
umount aosp
