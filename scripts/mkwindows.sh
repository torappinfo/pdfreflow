#!/bin/sh
BIN="pdfreflow.exe"
OSDIR="windows"
TARFILE=${OSDIR}.tgz
DSTDIR="../tmp/${OSDIR}"

usage() {
	echo "usage: mk${OSDIR}"
	exit 1

}

if [  $# -ne 0  ] ; then
	usage
fi

DSTHOST=$1

mkdir -p $DSTDIR
cp ../src/${BIN} $DSTDIR
cd $DSTDIR
strip -S $BIN
cd ..
tar zcf $TARFILE $OSDIR
