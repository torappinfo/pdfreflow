#!/bin/sh

NAME=`grep PACKAGE_STRING ../config.h | sed -e "s/flow /flow-/"  -e "s/\"//g" | gawk ' { print $3 } '`
DSTDIR="../tmp/release/${NAME}"

rm -rf ${DSTDIR} ${DSTDIR}.zip
(cd ..; ls *.tgz > /tmp/list.txt);
mkdir -p $DSTDIR
cp -p ../pdfreflow.html ${DSTDIR}
cd $DSTDIR
DSTNAME=`pwd`
for i in `cat /tmp/list.txt`; do
    tar zxf ../../../${i}
done
cd ..
zip -r ${NAME}.zip ${NAME}
echo release saved in ${DSTNAME}.zip

