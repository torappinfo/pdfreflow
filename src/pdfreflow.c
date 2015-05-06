/*
 * pdfreflow.h 
 *
 * Copyright (C) 2010 Pranananda Deva 
 *
 * This file is part of pdfreflow.
 * 
 * Pdfreflow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Pdfreflow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with pdfreflow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include "html.h"
#include "rect.h"
#include "font.h"
#include "page.h"
#include "parse.h"
#include "array.h"
#include "hashtable.h"



static void
debugusage() {
    fprintf(stderr, "\n\
Debug usage: pdfreflow [debug_options] < inputfile \n\
Debugging Options:\n\
      --bounds		print bounding rect of each page\n\
  -C, --chapterfs=SIZE	minimum fontsize for chapter header (default is 20)\n\
      --chapters	print chapters found, showing page number\n\
      --fonts		print all the fonts in document\n\
      --fontsize	print font size frequency\n\
      --leading		print leading frequency\n\
      --left		print left text pos frequency\n\
      --lineheight	print lineheight frequency\n\
      --pageno		print discovered page numbers\n\
      --right		print right text pos frequency\n\
      --showpara	print paragraph debugging info\n\
      --showstyles	print style debugging info\n\
      --xbounds		print minx/maxx of text bounds for pages\n\
      --xml		print xml for page\n\
\n");
    exit(0);
}

 /*
  * usage
  *  print usage string
  */
static void
usage(int retval) {
    fprintf(stderr, "\n\
usage: pdfreflow [options] [inputfile] \n\
Options:\n\
      --absolute	font sizes are the same as the original document\n\
                        (not the default) the default is to remap fonts to\n\
			a relative size\n\
  -b, --bottom=MAXTOP	crop text whose top is greater than or equal to maxtop\n\
  -c, --center=SPEC	argument is page:line, ie 2:1 is line 1 on page 2\n\
			is a centered line (sometimes this hint is needed)\n\
  -d, --dontreflow=PAGES don't reflow comma separated page ranges,\n\
			i.e. \"1,2,4-9,100\"\n\
  -f, --first=FIRSTPAGE starting page (default is 1)\n\
  -l, --last=LASTPAGE	ending page (default is last page of the document)\n\
      --nonfiction	for books that use block quoting at the same inset\n\
			as the paragraph indent\n\
  -r, --ragright	text is rag-right, NOT justify (default is justify)\n\
  -s, --shortlines	paragraphs end with short lines (only necessary\n\
			for rag right documents with no paragraph indent\n\
			and no after paragraph vertical spacing.\n\
  -t, --top=MINTOP	crop text whose top is less than or equal to mintop\n\
\n\
      --showdebug	print debugging options\n\
  -v, --version		print current version\n\
  -?, --help		print this help\n\
\n\
If inputfile is specified, file suffix is replace with .html and the \n\
ouput is written to that file. I.E. an input file of ~/mybook.xml has an\n\
output file ~/mybook.html. \n\
\n\
If no input file is specified, stdin used as the input, and stdout is\n\
the output.\n\
\n");
    exit(retval);
}


static int absolute = 0;
static int fontsize = 20;
static int showchapters = 0;
static int showpagenums = 0;
static int showfonts = 0;
static int showfontsize = 0;
static int showleading = 0;
static int showlineheight = 0;
static int showleft = 0;
static int showright = 0;
static int firstpage = 0;
static int lastpage = -1;
static int showxml = 0;
static int showbounds = 0;
static int showx = 0;
static char *inputname = NULL;
static int showstyles = 0;
static int showpara = 0;
static int showdebug = 0;
static int showhelp = 0;
static int center_page = 0;
static int center_line = 0;
static int printpage = 0;
static int nonfiction = 0;
static int shortlines = -1;

 /* argument for getopt_long, for command line processing */
struct option longopts[] = {
    { "absolute",   no_argument, &absolute, 1 },
    { "bottom",	    required_argument, NULL, 'b' },
    { "bounds",	    no_argument, &showbounds, 1 },
    { "center",	    required_argument, NULL, 'c' },
    { "chapterfs",  required_argument, NULL, 'C' },
    { "chapters",   no_argument, &showchapters, 1 },
    { "dontreflow", required_argument, NULL, 'd' },
    { "first",	    required_argument, NULL, 'f' },
    { "fonts",	    no_argument, &showfonts, 1 },
    { "fontsize",   no_argument, &showfontsize, 1 },
    { "help",	    no_argument, &showhelp, 1 },
    { "last",	    required_argument, NULL, 'l' },
    { "leading",    no_argument, &showleading, 1 },
    { "lineheight", no_argument, &showlineheight, 1 },
    { "left",	    no_argument, &showleft, 1 },
    { "nonfiction", no_argument, &nonfiction, 1 },
    { "pageno",	    no_argument, &showpagenums, 1 },
    { "print",	    required_argument, NULL, 'p' },
    { "ragright",   no_argument, NULL, 'r' },
    { "shortlines", required_argument, NULL, 's' },
    { "showdebug",   no_argument, &showdebug, 1 },
    { "showpara",   no_argument, &showpara, 1 },
    { "showstyles", no_argument, &showstyles, 1 },
    { "right",	    no_argument, &showright, 1 },
    { "top",	    required_argument, NULL, 't' },
    { "xbounds",    no_argument, &showx, 1 },
    { "xml",	    no_argument, &showxml, 1 },
    { "version",    no_argument, NULL, 'v' },
    {0}
};

 /*
  * showversion
  *  prints the current version (from config.h) to stderr
  */
static void showversion() {
    fprintf(stderr, "version: %s\n", PACKAGE_STRING);
    exit(0);
}

 /*
  * parsecenter
  *  parses the --center argument, a : delimited field
  *  the first integer is the page number (starting at 1)
  *  the 2nd integer is the line number (starting at 1)
  */
static void parsecenter(char *str) {
    char *colon = strchr(str, ':');
    if (!colon) {
	fprintf(stderr, "missing colon in center arg:%s\n", str);
	exit(3);
    }
    *colon = 0;
    center_page = strtol(str, NULL, 10);
    if (!center_page) {
	fprintf(stderr, "invalid page number: %s\n", str);
	exit(3);
    }
    center_line = strtol(colon + 1, NULL, 10);
    if (!center_line) {
	fprintf(stderr, "invalid line number: %s\n", colon + 1);
	exit(3);
    }
}

 /*
  * readargs
  *  parse command line arguments into a bunch of statics
  */
static void readargs(int argc, char **argv) {
    int ch;
    while ((ch = getopt_long(argc, argv, "rvb:cC::d:f:l:p:s:t:", longopts, NULL)) != EOF) {
	switch (ch) {
	    case 'b':
		page_cropbottom(strtol(optarg, NULL, 10));
		break;
	    case 0:
		break;
	    case 'c':
		parsecenter(optarg);
		break;
	    case 'C':
		fontsize = strtol(optarg, NULL, 10);
		break;
	    case 'd':
		setPageRange(optarg);
		break;
	    case 'f':
		firstpage = strtol(optarg, NULL, 10) - 1;
		break;
	    case 'l':
		lastpage = strtol(optarg, NULL, 10);
		break;
	    case 'p':
		printpage = strtol(optarg, NULL, 10);
		break;
	    case 'r':
		page_setrag();
		break;
	    case 's':
		shortlines = strtol(optarg, NULL, 10);
		break;
	    case 't':
		page_croptop(strtol(optarg, NULL, 10));
		break;
	    case 'v':
		showversion();
		break;
	    default:
		usage(1);
		break;
	}
    }
    argc -= optind;
    argv += optind;
    if (firstpage < 0) {
	fprintf(stderr, "Invalid first page %d\n", firstpage);
	usage(1);
    }
    if (lastpage >= 0 && lastpage < firstpage) {
	fprintf(stderr, "Invalid lastpage page %d\n", lastpage);
	usage(1);
    }
    if (argc)
	inputname = argv[0];
    if (showhelp)
	usage(0);
    if (showdebug)
	debugusage();
}

 /*
  * printbounds
  *  print bounds of pages
  */
static void printbounds(struct array *pages) {
    int i, length = array_length(pages);
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = firstpage; i < length; i++) {
	struct Page *page = array_elementat(pages, i);
	struct Rect r = page_textbounds(page);
	fprintf(stderr, "page %3d: t: %3d, l:%3d, w:%3d, h:%3d\n", i+1, r.top, r.left, r.width, r.height);
    }
}

 /*
  * printxml
  *  print xml content of pages
  */
static void printxml(struct array *pages) {
    int i, length = array_length(pages);
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = firstpage; i < length; i++) {
	struct Page *page = array_elementat(pages, i);
	page_print(page);
    }
}

 /*
  * printpagenums
  *  print the page number found in the text of the pages
  */
static void printpagenums(struct array *pages) {
    int i, length = array_length(pages);
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = firstpage; i < length; i++) {
	struct Page *page = array_elementat(pages, i);
	page_printpageno(page);
    }
}

 /*
  * printchapters
  *  print the page number of chapter headings based on font size heuristic
  */
static void printchapters(struct array *pages) {
    int i, length = array_length(pages);
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = firstpage; i < length; i++) {
	struct Page *page = array_elementat(pages, i);
	page_printchapter(page, fontsize);
    }
}

 /*
  * frequency
  *  struct used for frequency analysis
  */
struct frequency {
    int val;		/* value of stuff being analyzed for frequency */
    int count;		/* the number of value that occurred */
};

 /*
  * frequency_compare
  *  qsort call back to sort frequency in ascending orders
  */
static int frequency_compare(const void *p1, const void *p2) {
    struct frequency *s1 = (struct frequency *)p1;
    struct frequency *s2 = (struct frequency *)p2;
    if (s1->count < s2->count)
	return 1;
    else if (s2->count < s1->count)
	return -1;
    if (s1->val < s2->val)
	return -1;
    if (s1->val > s2->val)
	return 1;
	
    return 0;
}

#define FREQSIZE 3	/* the number of saved frequency values from the last call to printfreq */

static struct frequency lastfreq[FREQSIZE]; /* saved frequency values */

 /*
  * printfreq
  *  do frequency analysis, and print up to 10 of the max frequencies found
  */
static int printfreq(struct hashtable *hash, char *title, int print, int val_is_str, int bias) {
    int i, length;
    int retval = -1;
    struct array *keys, *vals;
    struct frequency keyval;
    struct frequency *ptr;

    keys = hashtable_keys(hash, 0);
    vals = array_init(sizeof(struct frequency), 0);
    length = array_length(keys);
    for (i = 0; i < length; i++) {
	void *key = array_elementat(keys, i);
	void *val = hashtable_get(hash, key);
	keyval.val = (int)key;
	keyval.count = (int)val;
	array_append_element(vals, &keyval);
    }
    qsort(array_elementat(vals, 0), length, sizeof(struct frequency), frequency_compare);
/*
    if (length > 10)
	length = 10;
*/
    memset(lastfreq, 0, sizeof(lastfreq));
    
    for (i = 0; i < length; i++) {
	ptr = array_elementat(vals, i);
	if (i == 0) 
	    retval = ptr->val - bias;
	if (i < FREQSIZE) {
	    lastfreq[i].val = ptr->val - bias;
	    lastfreq[i].count = ptr->count;
	}
	if (print) {
	    if (val_is_str)
		fprintf(stderr, "%s: %s, count: %d\n", title, (char *)ptr->val, ptr->count);
	    else
		fprintf(stderr, "%s: %d, count: %d\n", title, ptr->val - bias, ptr->count);
	}
    }
    array_free(vals);
    array_free(keys);
    return retval;
}

 /*
  * checkindent
  *  this must be called right after analyzeleft to work, as it uses some statics that were
  *  left behind by printfreq.
  *  tries some heuristics to see if we can deduce what the left indent xposition is.
  */
static void checkindent(int lineheight) {
    int indent_to_line_ratio, indent, leftmargin;
    int linedelta, delta = 30;
    int marginindex = 1;
    
    /* if there is only xpos in the whole doc, then there can't be any left indent */
    if (lastfreq[1].count == 0)
	return;
    /* there should be more than 1/14 the number of leftmargin lines */
    indent_to_line_ratio = lastfreq[0].count / 14;
    if (lastfreq[1].count < indent_to_line_ratio)
	return;
    /* see if the 3rd value is a better fit. If so, try it instead */
    if (lastfreq[2].count != 0 && lastfreq[2].count >= indent_to_line_ratio) {
	marginindex = 2;
    }
    /*
     * calculate a delta based on lineheight, roughly 5 spaces, where the width is 60% of
     * lineheight
     */
    linedelta = (5 * lineheight * 6) / 10;
    if (linedelta > delta)
	delta = linedelta;
    indent = lastfreq[marginindex].val;
    leftmargin = lastfreq[0].val;
    /* is indent reasonable (within 5 spaces of left margin, greater than leftmargin) */
    if (indent <= leftmargin || indent > (leftmargin + delta))
	return;
    page_setindent(indent);
}

 /*
  * gethash
  *  returns a hashtable where the keys are pointers
  */
static struct hashtable *gethash() {
    struct hashtable *hash = hashtable_create(0);
    hashtable_set_keys_are_pointers(hash);
    return hash;
}

 /*
  * enumpages
  *  enumerate though pages and collect information for frequency analysis
  */
static int enumpages(struct array *pages, void (*proc)(struct Page *page, struct hashtable *hash), char *title, int print, int val_is_str, int bias) {
    int i, length = array_length(pages);
    int retval = -1;
    struct hashtable *hash = gethash();
    int start = 0;
    
    start = firstpage;
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = start; i < length; i++) {
	struct Page *page = array_elementat(pages, i);
	proc(page, hash);
    }
    retval = printfreq(hash, title, print, val_is_str, bias);
    hashtable_free(hash);
    return retval;
}

 /*
  * getleft
  *  callback to get left position for --left debug option
  */
static void getleft(struct Page *page, struct hashtable *hash) {
    page_getleft(page, hash);
}

 /*
  * getright
  *  callback to get right position for --right debug option
  */
static void getright(struct Page *page, struct hashtable *hash) {
    page_getright(page, hash);
}

 /*
  * getfontsize
  *  callback to get right position for --fontsize debug option
  */
static void getfontsize(struct Page *page, struct hashtable *hash) {
    page_getfontsize(page, hash);
}

#define LEADING_BIAS 100
 /*
  * getleading
  *  callback to get line leading position for --leading debug option
  */
static void getleading(struct Page *page, struct hashtable *hash) {
    page_getleadings(page, hash, LEADING_BIAS);
}

 /*
  * analyzeleading
  *  get the most frequent leading, and possibly print some debugging info
  */
static int analyzeleading(struct array *pages, int print) {
    return enumpages(pages, getleading, "leading", print, 0, LEADING_BIAS);
}

 /*
  * getlineheight
  *  callback to get lineheight position for --lineheight debug option
  */
static void getlineheight(struct Page *page, struct hashtable *hash) {
    page_getlineheight(page, hash);
}

 /*
  * analyzelineheight
  *  get the most frequent lineheight, and possibly print some debugging info
  */
static int analyzelineheight(struct array *pages, int print) {
    return enumpages(pages, getlineheight, "height", print, 0, 0);
}

 /*
  * analyzeleft
  *  get the most frequent left position, and possibly print some debugging info
  */
static int analyzeleft(struct array *pages, int print) {
    return enumpages(pages, getleft, "left", print, 0, 0);
}

 /*
  * analyzeright
  *  get the most frequent right position, and possibly print some debugging info
  */
static int analyzeright(struct array *pages, int print) {
    return enumpages(pages, getright, "right", print, 0, 0);
}

 /*
  * analyzefontsize
  *  get the most frequent fontsize, and possibly print some debugging info
  */
static int analyzefontsize(struct array *pages, int print) {
    return enumpages(pages, getfontsize, "fontsize", print, 1, 0);
}

 /*
  * printx
  *  print minx/maxx of pages
  */
static void printx(struct array *pages) {
    int i, length = array_length(pages);
    struct hashtable *mins = gethash();
    struct hashtable *maxs = gethash();
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = firstpage; i < length; i++) {
	struct Page *page = array_elementat(pages, i);
	struct Bounds b = rect_to_bounds(page_textbounds(page));
	fprintf(stderr, "page %3d: minx: %3d, maxx:%3d\n", i+1, b.minx, b.maxx);
	put_int(mins, b.minx, 1);
	put_int(maxs, b.maxx, 1);
    }
    fprintf(stderr, "\nmin x freq:\n");
    printfreq(mins, "minx", 1, 0, 0);
    fprintf(stderr, "\nmax x freq:\n");
    printfreq(maxs, "maxx", 1, 0, 0);
    hashtable_free(mins);
    hashtable_free(maxs);
}

 /*
  * getoutputfile
  *  open output file replacing .xml with .html
  *  if no file name, return standard out
  */
static FILE *getoutputfile() {
    char name[1024], *period;
    FILE *file;
    if (!inputname)
	return stdout;
    strcpy(name, inputname);
    period = strrchr(name, '.');
    if (!period)
	strcat(name, ".html");
    else
	strcpy(period, ".html");
    file = fopen(name, "w");
    if (!file) {
	fprintf(stderr, "Unable to open %s for writing.\n", name);
	exit(1);
    }
    return file;
}

 /*
  * findcenter
  *  uses the --center argument to discover the center X pos of the document
  */
static void findcenter(struct array *pages) {
    struct Page *page = array_elementat(pages, center_page - 1);
    if (!page) {
	fprintf(stderr, "Invalid page number: %d\n", center_page);
	exit(3);
    }
    page_setcenter(page, center_line - 1);
}

 /*
  * printpara
  *  print paragraph groups in pages
  */
static void printpara(struct array *pages) {
    int i, lineheight, length = array_length(pages);
    struct array *texts = array_pointer_init(0);
    struct hashtable *pagestarts = hashtable_create(0);
    struct Rect maxbounds = {0};
    struct Rect bounds = {0};
    struct Page *page;
    struct Rect minbounds = {0};
    struct Atom *defaultfont;
    FILE *file = getoutputfile();
    
    if (center_line && center_page)
	findcenter(pages);
    lineheight = analyzelineheight(pages, 0);
    minbounds.left = analyzeleft(pages, 0);
    checkindent(lineheight);
    minbounds.width = analyzeright(pages, 0) - minbounds.left;
    page_setlineleading(analyzeleading(pages, 0));
    page_setlineheight(lineheight);
    defaultfont = (struct Atom *)analyzefontsize(pages, 0);
    html_setdefaultfont(defaultfont);
    hashtable_set_keys_are_pointers(pagestarts);
    if (lastpage > 0 && lastpage < length)
	length = lastpage;
    for (i = firstpage; i < length; i++) {
	page = array_elementat(pages, i);
	page_initpara(page, minbounds);
    }
    for (i = firstpage; i < length; i++) {
	page = array_elementat(pages, i);
	bounds = page_textbounds(page);
	page_printpara(page, minbounds, texts, pagestarts);
	if (i == firstpage)
	    maxbounds = bounds;
	else
	    maxbounds = rect_add(maxbounds, bounds);
    }
    page_multipleparas(file, texts, pagestarts, minbounds);
}


 /*
  * printsummary
  *  print encoding and number of pages parsed and processed
  */
static void printsummary(struct array *pages) {
    int length = array_length(pages);
    int viewedpages = length;

    if (lastpage > 0 && lastpage < length)
	viewedpages = lastpage - firstpage;
    fprintf(stderr, "Encoding: %s\n", parse_encoding());
    fprintf(stderr, "%d pages parsed, %d pages processed.\n", length, viewedpages);
}

 /*
  * getinputfile
  *  get input file. If not input name specified, use standard in
  */
static FILE *getinputfile() {
    FILE *file;
    if (!inputname)
	return stdin;
    file = fopen(inputname, "r");
    if (!file) {
	fprintf(stderr, "Unable to open input file %s.\n", inputname);
	exit(1);
    }
    return file;
}

 /*
  * printpagecontent
  *  print the text lines of a page
  */
static void printpagecontent(struct array *pages) {
    int length = array_length(pages);
    struct Page *page;
    int realpage = printpage - 1;
    if (realpage < 0 || realpage >= length) {
	fprintf(stderr, "page %d is out of range (1 - %d)\n", printpage, length + 1);
	exit(2);
    }
    page = array_elementat(pages, realpage);
    page_printcontents(page, stderr);
}

 /*
  * main - parse command line, parse xml file, and execute various options
  */
int main(int argc, char **argv) {
    struct array *pages;
    
    readargs(argc, argv);
    pages = parse_pdf2xml(getinputfile());
    if (pages) {
	if (shortlines >= 0)
	    page_setshortlines(shortlines ? shortlines : 80);
	if (nonfiction)
	    page_setnonfiction();
	if (absolute)
	    html_setabsolute();
	if (showstyles)
	    html_setshowstyle();
	if (showpara)
	    page_setshowpara();
	printsummary(pages);
	if (showpagenums)
	    printpagenums(pages);
	if (showchapters)
	    printchapters(pages);
	if (showfonts)
	    font_printfonts();
	if (showfontsize)
	    analyzefontsize(pages, 1);
	if (showleading)
	    analyzeleading(pages, 1);
	if (showlineheight)
	    analyzelineheight(pages, 1);
	if (showleft)
	    analyzeleft(pages, 1);
	if (showright)
	    analyzeright(pages, 1);
	if (showxml)
	    printxml(pages);
	if (showbounds)
	    printbounds(pages);
	if (showx)
	    printx(pages);
	if (printpage)
	    printpagecontent(pages);
	if (!(showpagenums + showchapters + showfonts + showfontsize + 
	    showleft + showright + showxml + showbounds + showleading +
	    showx + showlineheight + printpage)  || inputname)
	    printpara(pages);
	array_free(pages);
    }
    return 0;
}
