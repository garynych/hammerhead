#!/bin/bash

# build script
export ARCH=arm;
export SUB_ARCH=arm;
export USER=`whoami`;
export TMPFILE=`mktemp -t`;
export KBUILD_BUILD_USER="TomorrowLandAce";
export KBUILD_BUILD_HOST="PokeCenter";
export NUMBEROFCPUS=`grep 'processor' /proc/cpuinfo | wc -l`;  
export CROSS_COMPILE=~/Android/toolchain/arm-eabi-4.8/bin/arm-eabi-;


if [ $# -gt 0 ]; then
    echo $1 > .version
fi

if [ ! -f .config ]; then
    echo "Clean Build Started !"
    cp arch/arm/configs/tla_defconfig .config;
    make tla_defconfig;
fi;

res1=$(date +%s.%N)
echo "Starting Build !"

make -j$NUMBEROFCPUS CONFIG_NO_ERROR_ON_MISMATCH=y CONFIG_DEBUG_SECTION_MISMATCH=y

cp arch/arm/boot/zImage-dtb ../ramdisk/

cd ../ramdisk/

echo "Making ramdisk !"
./mkbootfs boot.img-ramdisk | gzip > ramdisk.gz
echo "Making bootimg !"
./mkbootimg --kernel zImage-dtb --cmdline 'console=ttyHSL0,115200,n8 androidboot.hardware=hammerhead user_debug=31 msm_watchdog_v2.enable=1' --base 0x00000000 --pagesize 2048 --ramdisk_offset 0x02900000 --tags_offset 0x02700000 --ramdisk ramdisk.gz --output ../aosp/boot.img

rm -rf ramdisk.gz
rm -rf zImage

cd ../aosp/

zipfile="CharizardX-r47test-hammerhead.zip.zip"
echo "Making flashable zip!"
cp boot.img zip/

rm -rf ../ramdisk/boot.img

cd zip/
rm -f *.zip
zip -r -9 $zipfile *
rm -f /tmp/*.zip
cp *.zip /tmp

echo "READY TO FIGHT !!! \n\n"

res2=$(date +%s.%N)
echo "Total time elapsed: $(echo "($res2 - $res1) / 60"|bc ) minutes ($(echo "$res2 - $res1"|bc ) seconds)";
