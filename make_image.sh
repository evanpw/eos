#!/bin/bash
set -e
BUILD_DIR=$1

rootdir=$(mktemp -d)
cp $BUILD_DIR/user.bin $rootdir
rm -f $BUILD_DIR/diskimg2
mke2fs -d $rootdir $BUILD_DIR/diskimg2 32M
