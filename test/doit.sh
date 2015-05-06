#!/bin/sh
PATH=.:../src:$PATH

# TESTCOUNT is the number of tests
TESTCOUNT=11

# Here are the tests:
#   pdfreflow --pageno < test0.xml 2> out0.txt
#   pdfreflow test1.xml
#   pdfreflow test2.xml
#   pdfreflow --dontreflow='1-8,12' test3.xml
#   pdfreflow --ragright test4.xml
#   pdfreflow --top=39 --bottom=745 --dontreflow=4 --ragright test5.xml
#   pdfreflow --ragright test6.xml
#   pdfreflow --showpara --dontreflow=1-3 --ragright test7.xml
#   pdfreflow --ragright test8.xml
#   pdfreflow --showpara --top=182 --dontreflow='1-5,217-222' test9.xml
#   pdfreflow --showpara test10.xml
#

#init args array and stdin array, ie stdin3 means test 3 uses standard in.
initarray() {
    args0="--pageno"
    stdin0=1
    args3="--dontreflow='3-8,12' --center=1:2 --nonfiction"
    args4="--ragright"
    args5="--top=39 --bottom=745 --dontreflow=4 --ragright"
    args6="--ragright"
    args7="--dontreflow=1-3 --ragright  --center=3:2"
    args8="--ragright"
    args9="--dontreflow='1-5,217-222' --nonfiction"
}


cd ../test

# delete old tests
rm out*.txt *.html
initarray
CURTEST=0;
while [ $CURTEST -lt $TESTCOUNT ] ; do
    eval VAL="\$stdin${CURTEST}"
    if [ $VAL ] ; then
	eval echo Test ${CURTEST} pdfreflow --showpara \$args${CURTEST} "\<" test${CURTEST}.xml "2\>" out${CURTEST}.txt
	eval pdfreflow --showpara \$args${CURTEST} < test${CURTEST}.xml 2> out${CURTEST}.txt
    else
	eval echo Test ${CURTEST} pdfreflow --showpara \$args${CURTEST} test${CURTEST}.xml 2> out${CURTEST}.txt
	eval pdfreflow --showpara \$args${CURTEST} test${CURTEST}.xml 2> out${CURTEST}.txt
    fi
    if diff -qw out${CURTEST}.txt result${CURTEST}.txt ; then
	echo "Test ${CURTEST} succeeded"
    else
	echo "Test ${CURTEST} failed"
	echo diff -w result${CURTEST}.txt out${CURTEST}.txt
	diff -w result${CURTEST}.txt out${CURTEST}.txt
    fi
    CURTEST=`expr $CURTEST + 1`
done

#pdfreflow --pageno < test1.xml 2>out1.txt
