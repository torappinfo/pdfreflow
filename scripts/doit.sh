#!/bin/sh
PATH=.:../src:$PATH

cd ../scripts

pdfreflow --showpara --top=34 --dontreflow="1-6,17-20" ~/Documents/xml/IAmThat.xml 2> IAmThat.txt
open -a Google\ Chrome ~/Documents/xml/IAmThat.html

pdfreflow --showpara --bottom=540 --dontreflow="7-8"  ~/Documents/xml/grm.xml 2> grm.txt
open -a Google\ Chrome ~/Documents/xml/grm.html

pdfreflow --showpara --top=39 --bottom=745 --dontreflow=4 --ragright ~/Documents/xml/selfknow.xml 2> selfknow.txt
open -a Google\ Chrome ~/Documents/xml/selfknow.html

pdfreflow --showpara --ragright ~/Documents/xml/kundalini.xml 2> kundalini.txt
open -a Google\ Chrome ~/Documents/xml/kundalini.html


pdfreflow --showpara --top=182 --dontreflow="1-5,217-222"  ~/Documents/xml/secrets.xml 2> secrets.txt
open -a Google\ Chrome ~/Documents/xml/secrets.html


pdfreflow --showpara --dontreflow="1-8,12" ~/Documents/xml/sayings.xml 2> sayings.txt
open -a Google\ Chrome ~/Documents/xml/sayings.html

pdfreflow --showpara --ragright -dontreflow=3 ~/Documents/xml/mtm.xml 2> mtm.txt
open -a Google\ Chrome ~/Documents/xml/mtm.html

pdfreflow --showpara --ragright --dontreflow=3 ~/Documents/xml/doc.xml  2> doc.txt
open -a Google\ Chrome ~/Documents/xml/doc.html
