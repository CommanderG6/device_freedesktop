LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := \
	android-common \
	android-support-v13
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SDK_VERSION := current
LOCAL_PACKAGE_NAME := AutoLauncher
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_OVERRIDES_PACKAGES := \
	Home \
	Launcher3QuickStep \
	LatinIME \
	QuickSearchBox \
	SystemUI \
   \
   BasicDreams \
   Bluetooth \
   BluetoothMidiService \
   BookmarkProvider \
   Browser2 \
   BuiltInPrintService \
   Calendar \
   Camera2 \
   CaptivePortalLogin \
   CertInstaller \
   CompanionDeviceManager \
   CtsShimPrebuilt \
   DeskClock \
   EasterEgg \
   Email \
   ExactCalculator \
   Gallery2 \
   HTMLViewer \
   KeyChain \
   LiveWallpapersPicker \
   Music \
   NfcNci \
   PacProcessor \
   PhotoTable \
   PrintRecommendationService \
   PrintSpooler \
   SecureElement \
   SimAppDialog \
   Traceur \
   WallpaperBackup \
   WAPPushManager \

include $(BUILD_PACKAGE)
