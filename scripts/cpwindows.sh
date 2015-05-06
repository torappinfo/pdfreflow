#!/bin/sh
BIN="pdfreflow.exe"
OSDIR="windows"
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

cd $DSTDIR
cd ..
scp -p $TARFILE ${USER}@${DSTHOST}:/Users/${USER}/Documents/src/pdfreflow/
