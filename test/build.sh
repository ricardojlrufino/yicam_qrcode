
# Read enviroment
export $(cat env | xargs)

echo "Using Toolchain: $TOOLCHAIN_PATH"

export CROSSPATH=$TOOLCHAIN_PATH/bin
export STAGING_DIR=$TOOLCHAIN_PATH
export PATH="$CROSSPATH:$PATH"

export TARGET=arm-openwrt-linux-muslgnueabi
export CROSS=arm-openwrt-linux-muslgnueabi
export BUILD=x86_64-pc-linux-gnu
export CROSSPREFIX=${CROSS}-
export STRIP=${CROSSPREFIX}strip
export CXX=${CROSSPREFIX}g++
export CC=${CROSSPREFIX}gcc
export LD=${CROSSPREFIX}ld
export AS=${CROSSPREFIX}as
export AR=${CROSSPREFIX}ar
export RANLIB=${CROSSPREFIX}ranlib

make clean

make

if [ -f "$BIN" ]; then
    echo "Uploading to $TARGET_DEVICE:$TARGET_PATH"
    scp $BIN $TARGET_DEVICE:$TARGET_PATH
    echo "-- DONE --"
else
    echo "-- ERROR --"
fi

