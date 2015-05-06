#!/bin/sh

usage() {
	echo "usage: od rootname"
	exit 1

}

if [  $# -ne 1  ] ; then
	usage
fi


opendiff result${1}.txt out${1}.txt
