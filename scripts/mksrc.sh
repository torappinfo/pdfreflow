#!/bin/sh

NAME=`grep PACKAGE_STRING ../config.h | sed -e "s/flow /flow-/"  -e "s/\"//g" | gawk ' { print $3 } '`

DSTDIR="../../save/$NAME"
rm -rf $DSTDIR
rm -rf ${DSTDIR}.tgz
cp -rp ../../pdfreflow $DSTDIR
cd $DSTDIR
DIRNAME=`pwd`
echo "SRC=$DIRNAME"
echo "TGZ=${DIRNAME}.tgz"
make clean
(cd test; rm -rf out*.txt *.html)
(cd scripts; rm -rf *txt)
rm -rf config.h autom4te.cache *.tgz tmp downloads.txt
cd ..
tar -zcf ${NAME}.tgz ${NAME}


