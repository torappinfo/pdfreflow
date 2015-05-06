# This gawk script finds the center aligned paragraphs in all the pdfreflow output files
# c
# 
# Used in conjunction with center.sh

/page/ { curpage=$0 }
/center/ {
    split($5, vals, ",");
    if ($3 != vals[1] ) printf ("%s %s %s\n", FILENAME, curpage, $0)
}