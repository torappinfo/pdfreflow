#!/bin/sh
BIN="pdfreflow"
OSDIR="ubuntu"
TARFILE=${OSDIR}.tgz
DSTDIR="../tmp/${OSDIR}"

usage() {
	echo "usage: mk${OSDIR} user dest-host"
	exit 1

}

if [  $# -ne 2  ] ; then
	usage
fi

USER=$1
DSTHOST=$2

mkdir -p $DSTDIR
cp ../src/${BIN} $DSTDIR
cd $DSTDIR
strip -S $BIN
cd ..
tar zcf $TARFILE $OSDIR

scp -p $TARFILE ${USER}@${DSTHOST}:/Users/${USER}/Documents/src/pdfreflow/
