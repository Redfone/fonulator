#!/bin/bash

BUILD_NUM=`awk '/BUILD_NUM/ {print $3}' ver.h`
let BUILD_1=$BUILD_NUM+1
echo "Incrementing Build number from: "$BUILD_NUM" to "$BUILD_1

sed -i.backup -e 's/'$BUILD_NUM'/'$BUILD_1'/' ver.h
