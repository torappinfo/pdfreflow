#!/bin/sh

# This bourne shell script finds the center aligned paragraphs in all the pdfreflow output files
# the center aligned paragraphs in all the pdfreflow output files
# Used in conjunction with center.gawk


gawk -f center.gawk out*.txt