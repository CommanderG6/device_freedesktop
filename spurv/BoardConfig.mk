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

# Use the non-open-source parts, if they're present
-include vendor/freedesktop/spurv/BoardConfigVendor.mk

TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_VARIANT := generic
TARGET_USES_64_BIT_BINDER := true
TARGET_BOARD_PLATFORM := spurv
TARGET_COPY_OUT_VENDOR := vendor
TARGET_NO_RECOVERY := true
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

WITH_DEXPREOPT := true

BOARD_USES_VENDORIMAGE := true
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_PARTITION_SIZE := 100663296 # 96MB
BOARD_BUILD_SYSTEM_ROOT_IMAGE := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1258291200 # 1200MB
BOARD_USERDATAIMAGE_PARTITION_SIZE := 576716800
BOARD_FLASH_BLOCK_SIZE := 512
BOARD_HAVE_BLUETOOTH := false
BOARD_USES_GENERIC_AUDIO := true
BOARD_GPU_DRIVERS := etnaviv imx

VSYNC_EVENT_PHASE_OFFSET_NS := 0
USE_OPENGL_RENDERER := true
USE_CAMERA_STUB := true
USE_IIO_SENSOR_HAL := true
USE_IIO_ACTIVITY_RECOGNITION_HAL := true
NO_IIO_EVENTS := true

BOARD_SEPOLICY_DIRS += \
        device/freedesktop/spurv/sepolicy \
        system/bt/vendor_libs/linux/sepolicy
