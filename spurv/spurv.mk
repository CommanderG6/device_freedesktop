#
# Copyright 2014 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, device/freedesktop/spurv/device.mk)

PRODUCT_NAME := spurv
PRODUCT_DEVICE := spurv
PRODUCT_BRAND := Android
PRODUCT_MODEL := spurv
PRODUCT_MANUFACTURER := freedesktop

PRODUCT_SHIPPING_API_LEVEL := 21

PRODUCT_COPY_FILES += $(LOCAL_PATH)/init.spurv.rc:root/init.spurv.rc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/init.eth0.sh:system/bin/init.eth0.sh
PRODUCT_COPY_FILES += $(LOCAL_PATH)/init:root/sbin/init
PRODUCT_COPY_FILES += $(LOCAL_PATH)/os-release:root/usr/lib/os-release
PRODUCT_COPY_FILES += $(LOCAL_PATH)/launch-container.sh:root/launch-container.sh
PRODUCT_COPY_FILES += $(LOCAL_PATH)/manifest.xml:$(TARGET_COPY_OUT_VENDOR)/manifest.xml
PRODUCT_COPY_FILES += $(LOCAL_PATH)/init.spurv.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.spurv.rc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/cmdline:root/cmdline
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.software.app_widgets.xml:system/etc/permissions/android.software.app_widgets.xml
PRODUCT_COPY_FILES += \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    $(LOCAL_PATH)/media_codecs.xml:system/etc/media_codecs.xml \

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/sensor.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/sensor.rc \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \

PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.sf.lcd_density=150 \
    ro.hardware.gralloc=gbm \
    gralloc.gbm.device=/dev/dri/card1 \
    ro.hardware.hwcomposer=spurv \
    ro.boot.hardware=spurv \
    ro.crypto.state=unsupported \
    ro.sys.sdcardfs=false \
    drm.gpu.vendor_name=etnaviv \
    sys.init_log_level=7 \
    persist.logd.size=1024K \
    debug.sf.nobootanimation=1 \
    qemu.hw.mainkeys=1 \
    dalvik.vm.heapsize=36m \
    spurv.application=fishnoodle.canabalt \
    spurv.display_width=1920 \
    spurv.display_height=1080 \

PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.usejit=false \
    ro.radio.noril=yes \

