#!/bin/sh

usage() {
	echo "usage: mkresult.sh rootname"
	exit 1

}

if [  $# -ne 1  ] ; then
	usage
fi


echo mv out${1}.txt result${1}.txt
mv  out${1}.txt result${1}.txt
