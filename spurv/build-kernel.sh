set -ex

SCRIPT_DIR=`dirname "$0"`

export ARCH=arm
export CROSS_COMPILE="ccache arm-linux-gnu-" 

./scripts/kconfig/merge_config.sh arch/arm/configs/multi_v7_defconfig $SCRIPT_DIR/kernel.config

make -j8 zImage dtbs

