set -ex

AOSP_ROOT=/home/tomeu/zodiac-prefix/source/aosp
NFSROOT=/home/tomeu/nfsroot

cd $AOSP_ROOT
export TEMPORARY_DISABLE_PATH_RESTRICTIONS=true

if [ ! -e /usr/lib/locale/C.UTF-8 ]; then
    sudo ln -s /usr/lib/locale/C.utf8 /usr/lib/locale/C.UTF-8
fi

make -j12
#SANITIZE_TARGET=address make -j12

mkdir -p $NFSROOT/home/tomeu/aosp
cp $AOSP_ROOT/device/freedesktop/spurv/run.sh $NFSROOT/home/tomeu/.
cp $AOSP_ROOT/out/target/product/spurv/system.img $NFSROOT/home/tomeu/aosp.img
cp $AOSP_ROOT/out/target/product/spurv/vendor.img $NFSROOT/home/tomeu/vendor.img
cp $AOSP_ROOT/out/target/product/spurv/userdata.img $NFSROOT/home/tomeu/userdata.img
sync

