#!/bin/bash
make -j8
cp -f ./build/water_gcc.hex C:/Users/panhan/Desktop/hex文件/T1.5/water_T1.5_ip_"$*".hex
echo "build and copy over, IP is 192.168.1."$*""