#!/bin/sh

DIR=../../save/pdfreflow-0.8.5/src


for i in *.c; do
    if diff -qa ${DIR}/$i . ; then
	echo $i not changed
    else
	opendiff ${DIR}/$i .
    fi
done
