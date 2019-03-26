DEVICE_MANIFEST_FILE := $(LOCAL_PATH)/manifest.xml

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

PRODUCT_PACKAGES += \
    hwcomposer.spurv \
    gralloc.gbm \
    libGLES_mesa \
    gatekeeper.spurv \
    sensors.iio \
    activity_recognition.iio \
    audio.primary.spurv \
    alsa-conf \
    libasound_module_pcm_pulse \
    libasound_module_ctl_pulse \
    libasound_module_conf_pulse

PRODUCT_PACKAGES += vndk_package

PRODUCT_PACKAGES += dhcpclient

PRODUCT_PACKAGES += \
    android.hardware.keymaster@4.0-impl \
    android.hardware.keymaster@4.0-service

PRODUCT_PACKAGES += \
    android.hardware.gatekeeper@1.0-impl \
    android.hardware.gatekeeper@1.0-service

PRODUCT_PACKAGES += \
    android.hardware.graphics.mapper@2.0 \
    android.hardware.graphics.mapper@2.0-impl \
    android.hardware.graphics.allocator@2.0 \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.composer@2.1 \
    android.hardware.graphics.composer@2.1-impl \
    android.hardware.graphics.composer@2.1-service

PRODUCT_PACKAGES += \
	android.hardware.audio@2.0-impl \
	android.hardware.audio@2.0-service \
	android.hardware.audio.effect@2.0-impl \

PRODUCT_PACKAGES += \
    android.hardware.power@1.0-impl \
    android.hardware.power@1.0-service

PRODUCT_PACKAGES += \
    android.hardware.sensors@1.0-impl \
    android.hardware.sensors@1.0-service

PRODUCT_PACKAGES += \
    android.hardware.wifi@1.0-service \

PRODUCT_PACKAGES += \
    android.hardware.gnss@1.1-impl \

PRODUCT_PACKAGES += \
    android.hardware.bluetooth@1.0-service.btlinux \

PRODUCT_PACKAGES += \
    org.jfedor.frozenbubble \
    AutoLauncher

