/*
 * page.c 
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
#include "page.h"
#include "font.h"
#include "html.h"
#include "text.h"
#include "hashtable.h"
#include "array.h"
#include "linetable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Page {
    int number;		    /* page number */
    int width;		    /* width in pixels of text string */
    int height;		    /* height in pixels of text string */
    int sorted;		    /* 1 if texts have been sorted by position */
    int removedpageno;	    /* 1 if the page number and associated texts have been removed */
    int removedempties;	    /* 1 if empty text objects have been removed */
    int dontreflow;	    /* 1 if user specified page to not be reflowed */
    struct Text *pageno;    /* text that contains the page number */
    struct array *texts;    /* array of text objects */
    struct Rect r;	    /* cached textbounds */
    int pageleft;	    /* set of leftmost x pos, when there are multiple lines */
};

static int page_delta = -1;		    /* diff between real and printed page number */
static int page_first = -1;		    /* first page with a detected page number */
static int page_last = -1;		    /* last page with a detected page number */
static int page_mintop = -1;		    /* min y position used for cropping */
static int page_maxtop = -1;		    /* max y position used for cropping */
static int page_rag = 0;		    /* paragraphs contain rag-right text (vs justified) */
static int page_lineheight = 15;	    /* ordinary line height, most used in document */
static int page_lineleading = -1;	    /* ordinary line leading, most used in document */
static int page_showpara = 0;		    /* show debugging paragraph info */
static struct array *pageranges = NULL;	    /* used for --dontreflow option */
struct Rect page_setminbounds(struct Rect a, struct Rect min);	/* mimimum valid page bounds */
static int page_center = -1;
static int page_nonfiction = 0;
static int page_shortlines = 0;
#define CDELTA 20			    /* inset from margins for centered text detection */

struct PageRange {
    int first;	    /* first page of range, inclusive */
    int last;	    /* last page of range, inclusive */
};

 /*
  * page_setshowpara
  *  debugging option --showpara
  */
void page_setshowpara() {
    page_showpara = 1;
}

 /*
  * page_setnonfiction
  *  text does not have a lot of dialog, not dependent on indent
  */
void page_setnonfiction() {
    page_nonfiction = 1;
}

 /*
  * page_setshortlines
  *  break paragraphs on short lines
  */
void page_setshortlines(int percent) {
    page_shortlines = percent;
}

 /*
  * checkreflow
  *  check to see if number is among the dontreflow page ranges
  */
static int checkreflow(int number) {
    int length;
    struct PageRange *first, *last, *cur;
    if (!pageranges)
	return 0;
    length = array_length(pageranges);
    first = array_elementat(pageranges, 0);
    last = first + length;
    for (cur = first; cur < last; cur++) {
	if (number >= cur->first && number <= cur->last)
	    return 1;
    }
    return 0;
}

 /*
  * checkdash
  *  look for a dash separated page range in the --dontreflow option
  */
static void checkdash(char *cur) {
    char *dash = strchr(cur, '-');
    struct PageRange pr;
    if (dash) {
	*dash = 0;
	pr.first = strtol(cur, NULL, 10);
	pr.last = strtol(dash+1, NULL, 10);
    } else {
	pr.first = strtol(cur, NULL, 10);
	pr.last = pr.first;
    }
    array_append_element(pageranges, &pr);
}

 /*
  * setPageRange
  *  parse page ranges for --dontreflow option, ie "1-3,4,10, 99-102"
  *  very strict about input, no accomodation for whitespace
  */
void setPageRange(char *range) {
    char *cur = range, *comma = NULL;
    
    pageranges = array_init(sizeof(struct PageRange), 0);
    while ((comma = strchr(cur, ','))) {
	*comma = 0;
	checkdash(cur);
	cur = comma + 1;
    }
    checkdash(cur);
}

 /*
  * page_setlineleading
  *  sets the most common line leading in document
  */
void page_setlineleading(int leading) {
    page_lineleading = leading;
}

 /*
  * page_setlineheight
  *  sets the most common lineheight in document
  */
void page_setlineheight(int lineheight) {
    page_lineheight = lineheight;
}

 /*
  * page_setrag
  *  set no-justify, or rag-right
  */
void page_setrag() {
    page_rag = 1;
    html_setrag();
}
 /*
  * page_cropbottom
  *  crop text objects whose top is greater than or equal to "cropbottom"
  */
void page_cropbottom(int cropbottom) {
    page_maxtop = cropbottom;
}

 /*
  * page_croptop
  *  crop text objects whose top is less than  or equal to "croptop"
  */
void page_croptop(int croptop) {
    page_mintop = croptop;
}

 /*
  * page_freetext
  *  called by the array_free to free array element, which are text objects
  */
static void page_freetext(struct array *arr, void **element, void *context) {
    struct Text **textp = (struct Text ** )element;
    text_free(*textp);
}

 /*
  * page_procs
  *  procs for array_special_init
  */
struct array_procs page_procs = {
    NULL,
    NULL,
    page_freetext,
    NULL
};

 /*
  * page_init
  *  create a page object
  */
struct Page *page_init(int number, int width, int height) {
    struct Page *page = calloc(1, sizeof(struct Page));
    if (page) {
	page->number = number;
	page->width = width;
	page->height = height;
	page->sorted = 0;
	page->removedpageno = 0;
	page->removedempties = 0;
	page->pageno = 0;
	page->texts = array_special_init(&page_procs, 0, NULL);
	array_set_contains_pointers(page->texts, 1);
	page->dontreflow = checkreflow(number);
    }
    return page;
}


 /*
  * page_free
  *  frees a page object
  */
void page_free(struct Page *page) {
    array_free(page->texts);
    free(page);
}

 /*
  * page_length
  *  return the number of Texts in a page
  */
int page_length(struct Page *page) {
    return array_length(page->texts);
}

 /*
  * page_textat
  *  return Text at index
  */
struct Text *page_textat(struct Page *page, int index) {
    return array_elementat(page->texts, index);
}

 /*
  * page_addtext
  *  add a Text to a page
  */
void page_addtext(struct Page *page, struct Text *text) {
    array_append_element(page->texts, text);
}

 /*
  * printval
  *  prints the real pagenumber and the detected printed page number
  */
static void printval(struct Page *page, struct Text *text, int index, int print) {
    int val = text_numericvalue(text);
    if (page_delta < 0) {
	page_delta = page->number - val;
	page_first = page->number;
    }
    page_last = page->number;
    page->pageno = text;
    if (print)
	fprintf(stderr, "page %d displays page number %d (%d of %d).\n", page->number, val, index, array_length(page->texts));
}

 /*
  * page_nopagenumber
  *  prints the an error for when no page number is detected
  */
static void page_nopagenumber(struct Page *page) {
    fprintf(stderr, "page %d has no page number.\n", page->number);
}

 /*
  * textenum_ret
  *  return result for callback proc called by page_enumtexts
  */
enum textenum_ret {
    textenum_cont,		    /* continue with enumeration */
    textenum_stop_with_results,	    /* stop, and return current text in enumeration */
    textenum_stop_no_results	    /* stop, and return NULL */
};

 /*
  * cont_increasing
  *  end condition of loop for increment count
  */
static int cont_increasing(int i, int len) {
    return (i < len);
}

 /*
  * cont_decreasing
  *  end condition of loop for decrement count
  */
static int cont_decreasing(int i, int len) {
    return (i >= 0);
}
 /*
  * page_enumtexts
  *  enumerate text objects either in increasing or decreasing order
  *  proc returns whether to continue, return with current text, or return with NULL
  */
static struct Text *page_enumtexts(struct Page *page, void *context, enum textenum_ret (*proc)(struct Page *page, void *context, struct Text *text), int increasing) {
    int i, length = array_length(page->texts);
    enum textenum_ret retval;
    struct Text *text;
    int (*cont)(int index, int length);
    int startindex = 0, increment = 0;
    if (increasing) {
	startindex = 0; increment = 1; cont = cont_increasing;
    } else {
	startindex = length - 1; increment = -1; cont = cont_decreasing;
    }
    for (i = startindex; cont(i, length); i += increment) {
	text = array_elementat(page->texts, i);
	retval = proc(page, context, text);
	switch (retval) {
	    case textenum_cont: break;
	    case textenum_stop_with_results: return text;
	    case textenum_stop_no_results: return NULL;
	}
    }
    return NULL;
}

 /*
  * page_context
  *  context for enumeration of text objects
  *  
  */
struct page_context {
    int baseline;	/* baseline of current text object */
};


 /*
  * contains_page_no
  *  see if the current text is a potential valid page number
  *  
  */
enum textenum_ret contains_page_no(struct Page *page, void *p, struct Text *cur) {
    int isempty = text_isempty(cur);
    int val;
    struct page_context *context = (struct page_context *) p;
    if (isempty)
	return textenum_cont;
    val = text_numericvalue(cur);
    /* see if the current numeric value is in a reasonable page number range */
    if (val > 0 && val <= page->number) {
	/* if there is a valid page_delta value, use it to further check for 
	 * a reasonable page number range
	 */
	if (page_delta > 0) {
	    if (val >= (page->number - page_delta)) {
		return textenum_stop_with_results;
	    } else {
		return textenum_stop_no_results;
	    }
	}
	return textenum_stop_with_results;
    }
    if (!context->baseline)
	context->baseline = text_baseline(cur);
    if (context->baseline && (context->baseline != text_baseline(cur)))
	return textenum_stop_no_results;
    return textenum_cont;
}

 /*
  * searchforward
  *  search from the start of the texts forward to detect a printed page number
  *  
  */
static struct Text *searchforward(struct Page *page) {
    struct page_context context = {0};
    return page_enumtexts(page, &context, contains_page_no, 1);
}

 /*
  * searchbackward
  *  search from the end of the texts backwards to detect a printed page number
  *  
  */
static struct Text *searchbackward(struct Page *page) {
    struct page_context context = {0};
    return page_enumtexts(page, &context, contains_page_no, 0);
}

 /*
  * page_printpageno
  *  print to stderr where in the text the page number is
  */
void page_findpageno(struct Page *page, int print) {
    struct Text *first = searchforward(page);
    struct Text *last = searchbackward(page);
    struct Text *use = NULL;

    if (first && last) 
	use = (text_height(first) < text_height(last)) ? first : last;
    else
	use = (first) ? first : last;
    if (use)
	printval(page, use, array_element_index(page->texts, use), print);
    else {
	if (print)
	    page_nopagenumber(page);
    }
}

 /*
  * page_printpageno
  *  detect page number and print to stderr
  */
void page_printpageno(struct Page *page) {
    page_findpageno(page, 1);
}

 /*
  * linematches
  *  stores text object that are on the same line as a detected page number (header/footers usually)
  */
static struct array *linematches = NULL;

 /*
  * page_removepageno
  *  remove entire header/footer that contains the page number
  */
static void page_removepageno(struct Page *page) {
    int i, length;
    struct Text *text;
    if (page->removedpageno)
	return;
    if (!page->pageno)
	page_findpageno(page, 0);
    if (!linematches)
	linematches = array_pointer_init(0);
    if (page->pageno) {
	length = array_length(page->texts);
	for (i = 0; i < length; i++) {
	    text = array_elementat(page->texts, i);
	    if (text_intersects(page->pageno, text) || text == page->pageno)
		array_append_element(linematches, text);
	}
	length = array_length(linematches);
	for (i = 0; i < length; i++) {
	    struct Text *text = array_elementat(linematches, i);
	    array_remove_element(page->texts, text);
	}
	page->pageno = NULL;
	array_setlength(linematches, 0);
    }
    /* crop texts that are less that mintop and greater than maxtop */
    if (page_mintop >= 0 || page_maxtop >= 0) {
	length = array_length(page->texts);
	for (i = 0; i < length; i++) {
	    int top;
	    text = array_elementat(page->texts, i);
	    top = text_top(text);
	    if (top <= page_mintop)
		array_append_element(linematches, text);
	    else if (page_maxtop >= 0 && top >= page_maxtop)
		array_append_element(linematches, text);
	}
	length = array_length(linematches);
	for (i = 0; i < length; i++) {
	    struct Text *text = array_elementat(linematches, i);
	    array_remove_element(page->texts, text);
	}
	array_setlength(linematches, 0);
    }
    length = array_length(page->texts);
    page->removedpageno = 1;
}

 /*
  * textcompare
  *  qsort comparison proc
  */
static int textcompare(const void *p1, const void *p2) {
    struct Text **s1 = (struct Text **)p1;
    struct Text **s2 = (struct Text **)p2;
    return text_compare(*s1, *s2);
}

 /*
  * removeempties
  *  remove all the empty text objects from the page
  */
static void removeempties(struct Page *page) {
    static struct array *empties = NULL;
    int i, length;
    
    if (page->removedempties)
	return;
    if (!empties)
	empties = array_pointer_init(0);
    length = array_length(page->texts);
    for (i = 0; i < length; i++) {
	struct Text *text = array_elementat(page->texts, i);
	if (text_isempty(text))
	    array_append_element(empties, text);
    }
    length = array_length(empties);
    for (i = length - 1; i >= 0; i--) {
	struct Text *text = array_elementat(empties, i);
	array_remove_element(page->texts, text);
    }
    array_setlength(empties, 0);
    page->removedempties = 1;
}

 /*
  * page_sort
  *  sort text objects in page
  */
void page_sort(struct Page *page) {
    removeempties(page);
    page_removepageno(page);
    if (!page->sorted) {
	int length;
	
	length = array_length(page->texts);
	array_set_contains_pointers(page->texts, 0);
	qsort(array_elementat(page->texts, 0), length, sizeof(char *), textcompare);
	array_set_contains_pointers(page->texts, 1);
	page->sorted = 1;
    }
}

struct PageStats {
    int left;
    int hitsleft;
    int nlines;
};

 /*
  * countlines
  *  count the lines in page, and count the number of times that a left edge of 
  *  a line hits the page bounds.
  */
static int countlines(void *context, struct Line *cur, struct Line *next) {
    struct PageStats *stats = (struct PageStats *)context;
    if (cur->r.left == stats->left)
	stats->hitsleft++;
    stats->nlines++;
    return 1;
}

 /*
  * page_textbounds
  *  returns the minimum left position of all the text objects
  */
struct Rect page_textbounds(struct Page *page) {
    int i, length;
    struct Rect r = { 0};
    struct PageStats stats = {0};
    if (page->r.left)
	return page->r;
    page_sort(page);
    length = array_length(page->texts);
    for (i = 0; i < length; i++) {
	struct Text *text = array_elementat(page->texts, i);
	if (i == 0)
	    r = text_rect(text);
	else
	    r = rect_add(r, text_rect(text));
    }
    page->r = r;
    stats.left = r.left;
    linetable_enum_linetable(page->texts, &stats, 0, countlines);
    if (stats.hitsleft > 1 && stats.hitsleft > (stats.nlines/10))
	page->pageleft = r.left;
    return r;
}

 /*
  * put_int
  *  if leading exists in hashtable, increment value. 
  *  if force, then alway increment
  *  returns 1 if successful, 0 if nothing happened
  */
int put_int(struct hashtable *hash, int leading, int force) {
    int oldval = (int)hashtable_get(hash, (void *) leading);
    if (oldval || force) {
	oldval++;
	hashtable_put(hash, (void *) leading, (void *)oldval);
	return 1;
    }
    return 0;
}


 /*
  * page_enumlines
  *  sorts before calling linetable_enumlines
  */
static void page_enumlines(struct Page *page, void *context, int (*proc)(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex)) {
    page_sort(page);
    linetable_enumlines(page->texts, context, proc);
}

struct FindCenter {
    int curline;
    int lineno;
};

 /*
  * findcenter
  *  callback of page_enumlines, to discover the page_center used for discovering centered 
  *  lines.
  */
static int findcenter(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex) {
    struct FindCenter *info = (struct FindCenter *)context;
    if (info->curline == info->lineno) {
	page_center = rect.left + (rect.width/2);
	return 0;
    }
    info->curline++;
    return 1;
}

 /*
  * page_setcenter
  *  the line (zero base) on this page is a centered line
  */
void page_setcenter(struct Page *page, int lineno) {
    struct FindCenter info = {0};
    page_enumlines(page, &info, findcenter);
}

struct Leading {
    int bias;
    struct hashtable *hash;
};

 /*
  * getleadings
  *  called back to calc leading and store in hash table. some hacking here to 
  *  match approximate values, since there seems to be some off-by-one noise
  *  in some poorly formatted PDF files 
  */
static int getleadings(void *context, struct Line *cur, struct Line *next) {
    struct Leading *info = (struct Leading *) context;
    int leading;
    
    if (!next)
	return 1;
    leading = info->bias + (next->r.top - cur->r.top) - cur->r.height;
    if (leading > 0)
	put_int(info->hash, leading, 1);
    return 1;
}

 /*
  * getleft
  *  record the left pos of a text 
  */
static int getleft(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex) {
    struct hashtable *hash = (struct hashtable *)context;
    put_int(hash, rect.left, 1);
    return 1;
}

 /*
  * getright
  *  record the right pos of a text 
  */
static int getright(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex) {
    struct hashtable *hash = (struct hashtable *)context;
    put_int(hash, rect_right(rect), 1);
    return 1;
}

 /*
  * getfontsize
  *  record the font size info of a text 
  */
static int getfontsize(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex) {
    struct hashtable *hash = (struct hashtable *)context;
    put_int(hash, (int)text_fontprintstr(cur), 1);
    return 1;
}

 /*
  * getlineheight
  *  record the lineheight of a text 
  */
static int getlineheight(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex) {
    struct hashtable *hash = (struct hashtable *)context;
    put_int(hash, (int)text_height(cur), 1);
    return 1;
}

 /*
  * page_getleadings
  *  place leading values into the hashtable. key=leading, val=count
  */
void page_getleadings(struct Page *page, struct hashtable *hash, int bias) {
    struct Leading info;
    page_sort(page);
    info.hash = hash;
    info.bias = bias;
    linetable_enum_linetable(page->texts, &info, 0, getleadings);
}

 /*
  * page_getleft
  *  place left positions of text objects into the hashtable. key=left, val=count
  */
void page_getleft(struct Page *page, struct hashtable *hash) {
    page_enumlines(page, hash, getleft);
}

 /*
  * page_getright
  *  place right positions of text objects into the hashtable. key=right, val=count
  */
void page_getright(struct Page *page, struct hashtable *hash) {
    page_enumlines(page, hash, getright);
}

 /*
  * page_getfontsize
  *  place font sizes of text objects into the hashtable. key=fontsize, val=count
  */
void page_getfontsize(struct Page *page, struct hashtable *hash) {
    page_enumlines(page, hash, getfontsize);
}

 /*
  * page_getlineheight
  *  place line height of text objects into the hashtable. key=height, val=count
  */
void page_getlineheight(struct Page *page, struct hashtable *hash) {
    page_enumlines(page, hash, getlineheight);
}


struct para_info {
    int lineno;			    /* current line number */
    int startpara;		    /* line number of the start of paragarph */
    struct Line *startline;	    /* Line struct at start of paragraph */
    int paranum;		    /* current paragraph number */
    struct Rect bounds;		    /* bounds of all the text objects */
    int indentright;		    /* right margin of an indented paragraph */
    int outdentright;		    /* right margin of an outdented paragraph */
    int indentleft;		    /* left margin of an indented paragraph */
    int outdentleft;		    /* left margin of an outdented paragraph */
    struct Rect minbounds;	    /* rect of the first line of the paragraph */
    int shortlines;		    /* width of a line short enough to break para */
    enum para_style style;	    /* style of the current paragraph */
    struct hashtable *pagestarts;    /* holds the text objects that start a page */
    int startpage;		    /* the page number of the current paragraph */
    int startlineno;		    /* line number at the start of page */
    struct Page *lastseen;	    /* Page holding the current paragraph */
    int pageleft;		    /* leftmost X on page */
    int startindex;		    /* index of text object for start paragraph */
    struct array *texts;	    /* list of texts to be evaluated */
    int dontprint;		    /* set to 1 when nothing is to be printed  */
    int printerr;		    /* set to 1 when nothing is to be printed to stderr */
    int notestyle;		    /* 1 when styles are to be recorded with the html output */
    int printstyle;		    /* 1 when styles are to be written as html output */
    int dontreflow;    		    /* 1 when current page is not to be reflowed */
    FILE *file;			    /* output file for generated HTML */
};

  /*
  * approx_match
  *  checks if 2 values are approximately equal to each other within a delta
  */
static int approx_match(int pos1, int pos2, int delta) {
    if ((pos2 >= (pos1 - delta)) && (pos2 <= (pos1 + delta)))
	return 1;
    return 0;
}

static int page_indentleft = -1;    /* x position of an indent (only 1 per doc) */
static int page_outdentleft = -1;   /* x position of an outdent (2nd line of paragraph) */
static int page_prevstyle = -1;	    /* style of previous paragraph */

  /*
  * normalheight
  *  returns 1 if height is most likely part of a paragraph 
  */
static int normalheight(int height) {
    return (height <= ((page_lineheight*5)/4));
}

 /*
  * page_setindent
  *  sets the x pos an indented line
  */
void page_setindent(int indent) {
    page_indentleft = indent;
}

 /*
  * iscentered
  *  returns 1 if line is centered
  */
static int iscentered(struct para_info *info, struct Rect r) {
    int center = page_center;
    int center1 = r.left + (r.width/2);
    
    if (center > 0 && approx_match(center, center1, 10))
	return 1;
    if (center < 0)
	center = info->bounds.left + (info->bounds.width/2);
    return approx_match(center, center1, 10);
}

 /*
  * strictcenter
  *  checks additionally for left and right x pos to be inset from text margins by CDELTA
  */
static int strictcenter(struct para_info *info, struct Rect r) {
    if (r.left < rect_max(page_indentleft + 2, (info->bounds.left + CDELTA)))
	return 0;
    if (rect_right(r) > (rect_right(info->bounds) - CDELTA))
	return 0;
    return iscentered(info, r);
}


  /*
  * realcheckparastyle
  *  set info->style for a paragraph, called with there are at least 2 lines in a paragaph
  */
static void realcheckparastyle(struct para_info *info, struct Rect rect, int nextleft) {
    int curiscentered = strictcenter(info, rect);
    int firstiscentered = strictcenter(info, info->startline->r);
    if (info->startline->r.left < rect.left && (!curiscentered && !firstiscentered) && normalheight(rect.height)) {
	info->style = align_outdent;
	if (!approx_match(info->outdentright, rect_right(info->startline->r), 2))
	    info->outdentright = rect_right(info->startline->r);
	if (!approx_match(info->outdentleft, rect.left, 2))
	    info->outdentleft = rect.left;
	info->indentleft = info->startline->r.left;
    } else if (info->startline->r.left > (rect.left + 3) && normalheight(rect.height)) {
	info->style = align_indent;
	if (!approx_match(info->indentright, rect_right(info->startline->r), 2))
	    info->indentright = rect_right(info->startline->r);
	if (!approx_match(info->indentleft, info->startline->r.left, 2))
	    info->indentleft = info->startline->r.left;
    } else if (approx_match(info->startline->r.left, info->bounds.left, 3)) {
	info->style = align_left;
	if (!info->indentright) 
	    info->indentright = rect_right(info->startline->r);
    } else if (info->startline->r.left > info->bounds.left && approx_match(rect_right(info->startline->r), rect_right(info->bounds), 3) && approx_match(rect_right(rect), rect_right(info->bounds), 3))
	info->style = align_right;
    else if (iscentered(info, info->startline->r) && iscentered(info, rect))
	info->style = align_center;
    else if (info->startline->r.left > info->bounds.left && rect_right(info->startline->r) < rect_right(info->bounds) && approx_match(info->startline->r.left, rect.left, 3))
	info->style = align_blockquote;
    if (info->style == align_indent && page_indentleft < 0) {
	page_indentleft = info->startline->r.left;
    }
    if (info->style == align_outdent && page_outdentleft < 0) {
	page_outdentleft = rect.left;
    }
}
/*
  * checkparastyle
  *  set info->style for a paragraph
  */
static void checkparastyle(struct para_info *info, struct Rect rect, int nextleft) {
    
    if (info->style != align_unknown)
	return;
    if ((info->lineno == (info->startpara + 1)))
	realcheckparastyle(info, rect, nextleft);
}

 /*
  * simplecheck
  *  a check for non-reflowed pages, very basic
  */
static void simplecheck(struct para_info *info, struct Line *cur) {
    if (cur->r.left == info->pageleft) {
	info->style = align_left;
    } else if (iscentered(info, cur->r)) {
	info->style = align_center;
    } else if (cur->r.left > info->bounds.left && approx_match(rect_right(cur->r), rect_right(info->bounds), 10)) {
	info->style = align_center;
    } else {
	info->style = align_left;
    }
}


 /*
  * checkoneliner
  *  set info->style for a one line paragraph
  */
static void checkoneliner(struct para_info *info, struct Line *cur, struct Line *next, int nextleft) {
    int equalleft = 0;
    
    if (next) {
	equalleft = (info->startline->r.left == next->r.left);
    }
    if (info->style != align_unknown)
	return;
	
    if (info->dontreflow) {
	simplecheck(info, cur);
	return;
    }
    if (strictcenter(info, cur->r)) {
	info->style = align_center;
	return;
    } 
    if (info->dontreflow) {
	info->style = align_left;
    } else if (approx_match(info->startline->r.left, info->bounds.left, 3)) {
	if ((nextleft == info->bounds.left || nextleft < 0) && page_prevstyle == align_outdent && normalheight(cur->r.height))
	    info->style = align_outdent;
	else
	    info->style = align_left;
    } else if (info->startline->r.left == page_indentleft &&
	normalheight(cur->r.height))
	info->style = align_indent;
    else if (info->startline->r.left > info->bounds.left && approx_match(rect_right(info->startline->r), rect_right(info->bounds), 3))
	info->style = align_right;
    else if (iscentered(info, cur->r))
	info->style = align_center;
    else if (info->startline->r.left > info->bounds.left && rect_right(info->startline->r) < rect_right(info->bounds))
	info->style = align_blockquote;
}

/*
static int endsinpunc(struct para_info *info, int nextindex) {
    struct Text *text;
    if (nextindex < 0)
	nextindex = array_length(info->texts);
    text = array_elementat(info->texts, nextindex - 1);
    return text_endsinpunc(text);
}
static int rightjustified(struct Rect r, struct Rect bounds) {
    return ((rect_right(r) == rect_right(bounds)) &&
	    r.left > (rect_max(bounds.left, page_indentleft) + 30));

}
  */

static int haveshortlines(struct para_info *info, struct Line *cur, struct Line *next) {
    struct Text *end = array_elementat(info->texts, next->index - 1);
    if (text_startswithcap(next->text) && text_endsinpunc(end))
	return 1;
    return 0;
}

 /*
  * atparaend
  *  returns 1 if at the end of a paragraph
  */
static int atparaend(struct para_info *info, struct Line *cur, struct Line *next) {
    int right;
    if (info->dontreflow)
	return 1;
    if (!next->text)
	return 1;
    if (next->r.height > (cur->r.height + 1))
	return 1;
    if (next->r.height < (cur->r.height - 1))
	return 1;
    if (info->style == align_indent || info->style == align_left) {
	if (next->r.left == page_indentleft && cur->r.left != page_indentleft)
	    return 1;
	if (next->r.left > (cur->r.left + 5))
	    return 1;
	right = rect_right(cur->r);
	if (!page_rag && (right < (info->indentright - 10)))
	    return haveshortlines(info, cur, next);
	if (info->style == align_left && info->shortlines) {
	    if (cur->r.width < info->shortlines)
		return 1;
	    return text_startswithquote(next->text);
	}
    }
    if (info->style == align_blockquote) {
	if (next->r.left < (info->startline->r.left - 3))
	    return 1;
    }
    if (info->style == align_outdent) {
	if (next->r.left < (cur->r.left - 2)) {
	    return 1;
	}
	if (!page_rag && rect_right(cur->r) < (info->outdentright - 10))
	    return 1;
    }
    if (info->style == align_right) {
	if (rect_right(next->r) < (rect_right(info->bounds) - 2))
	    return 1;
	else if (next->r.left == info->bounds.left)
	    return 1;
    }
    /* look for a 1 line paragraph that is indented */
    if (info->startpara == info->lineno) {
	    
	/* 2 indented lines next to each other */
	if (cur->r.left == page_indentleft && next->r.left >= page_indentleft) {
	    if (!page_nonfiction)
		return 1;
	    return haveshortlines(info, cur, next);
	}
	/* center line, but next is indent */
	if ((normalheight(cur->r.height) && strictcenter(info, cur->r)) && (next->r.left == page_indentleft || next->r.left == info->bounds.left)) {
	    return 1;
	}
	right = rect_right(cur->r);
	/* a short line */
	if (!page_rag && cur->r.left == info->bounds.left && right < (rect_right(info->bounds) - 20 ))
	    return haveshortlines(info, cur, next);
	if (!page_rag && cur->r.left == page_indentleft && right < (rect_right(info->bounds) - 10 ))
	    return haveshortlines(info, cur, next);
	/* a left, short line, when --shortlines=X option is specified */
	if (cur->r.left == info->bounds.left && info->shortlines) {
	    if (cur->r.width < info->shortlines)
		return 1;
	    return text_startswithquote(next->text);
	}
	/* cur is right justified, next is not */
/*
	if (rightjustified(r, info->bounds) && !rightjustified(next->r, info->bounds))
	    return 1;
*/	    
	if (page_prevstyle == align_outdent) {
	    if (cur->r.left == info->indentleft && next->r.left == info->indentleft) {
		return haveshortlines(info, cur, next);
	    }
	}
	/* cur is left justified, next is left, have outdent, could be 1 line outdent */
	if (page_indentleft >= 0 && page_outdentleft >= 0) {
	    if (cur->r.left == info->bounds.left && next->r.left == info->bounds.left)
		if (haveshortlines(info, cur, next)) {
		    struct Line *third = next + 1;
		    if (!third->text)
			return 1;
		    if (third->r.left == page_outdentleft && !text_startswithcap(third->text))
			return 1;
		}
	}
    }
    return 0;
}

 /*
  * printpageheader
  *  prints debugging output of current page number
  */
static void printpageheader(struct para_info *info) {
    if (info->lastseen) {
	if (!info->dontprint && info->printerr && page_showpara)
	    fprintf(stderr, "page %d:\n", info->lastseen->number);
	info->lastseen = NULL;
    }
}

 /*
  * dontreflow
  *  sees if text objects has dontreflow set
  */
static int dontreflow(struct para_info *info, struct Text *next) {
    struct Page *page;
    if (!next)
	return 1;
    if (!info->pagestarts)
	return 0;
    page = hashtable_get(info->pagestarts, (void *)next);
    if (page)
	return page->dontreflow;
    return 0;
}

 /*
  * atpageend
  *  if cur and next are on differnt pages, and the distance between the bottom
  *  of cur and the bottom margin is greater than the lineheight, return 1.
  */
static int atpageend(struct para_info *info, struct Line *cur, struct Line *next) {
    struct Rect nextr = {0};
    int nextindex = -1;
    if (next) {
	nextr = next->r;
	nextindex = next->index;
    }
    if (dontreflow(info, next->text))
	return 1;
    if (nextr.top < cur->r.top) {
	struct Rect r = cur->r;
	struct Text *last;
	if (nextindex < 0)
	    nextindex = array_length(info->texts);
	last = array_elementat(info->texts, nextindex - 1);
	if (page_rag) {
	    if (text_endsinpunc(last)) {
		return 1;
	    }
	} else {
	    int oneliner = (info->startpara == info->lineno);
	    int nextiscent = iscentered(info, nextr);
	    if (text_endsinpunc(last)) {
		if (info->style == align_outdent && r.left == nextr.left)
		    return 0;
		if (info->style == align_indent && approx_match(nextr.left, info->bounds.left, 5))
		    return 0;
		/* check if next line is centered & inset */
		if ((info->style == align_outdent || info->style == align_indent || info->style == align_left || info->style == align_right) && 
		    (nextr.left > (info->bounds.left + CDELTA) && (rect_right(nextr) < rect_right(info->bounds) - CDELTA) && nextiscent)) 
		    return 0;
		return 1;
	    }
	    if (oneliner) {
		/* if we have an inset, centered line, and the next line isn't centered */
		if (strictcenter(info, info->startline->r)
		    && !nextiscent)
		    return 1;
	    }
/*
	    if (dist_to_bottom > (10 * r.height))
		return 1;
*/
	}
    }
    return 0;
}

 /*
  * paraboundingrect
  *  get the bounding rect of the paragraph
  */
struct Rect paraboundingrect(struct para_info *info, int index, int nextindex, int oneliner) {
    int i;
    struct Rect r;
    if (nextindex < 0)
	nextindex = array_length(info->texts);
    for (i = info->startindex; i < nextindex; i++) {
	struct Text *text = array_elementat(info->texts, i);
	if (i == info->startindex)
	    r = text_rect(text);
	else
	    r = rect_add(r,  text_rect(text));
    }
    if (info->style == align_indent || info->style == align_outdent ||
	info->style == align_left) {
	if (oneliner) {
	    r.width = rect_right(info->bounds) - r.left;
	}
    }
    return r;
}
 /*
  * printparatexts
  *  print some debugging stuff to stderr about the paragraph structure
  *  print out HTML if info->pagestarts is set, if not set, then record style
  *  information
  */
static void printparatexts(struct para_info *info, struct Line *cur, struct Line *next, int have_end_spacing) {
    int other = -1, oldright;
    struct Rect r;
    int oneliner = (info->startpara == info->lineno);
    int nextindex = next ? next->index : -1;
    int index = cur->index;
    
    if (oneliner)
	checkoneliner(info, cur, next, have_end_spacing ? -1 : next->r.left);
    if (info->lastseen && info->startpage == info->lastseen->number) {
	printpageheader(info);
	info->lineno -= (info->startlineno - 1);
	info->startpara -= (info->startlineno - 1);
	info->paranum = 1;
    }
    if (!info->dontprint && info->printerr && page_showpara)
	fprintf(stderr, "  para %d: %d - %d, %s\n", info->paranum, info->startpara, info->lineno, html_parstylestr(info->style));
    r = paraboundingrect(info, index, nextindex, oneliner);
    switch (info->style) {
	case align_indent:
	    other = info->startline->r.left - info->bounds.left; 
	    break;
	case align_outdent:
	    other = r.left - info->outdentleft;
	    oldright = rect_right(r);
	    r.left = info->outdentleft;
	    r.width = oldright - r.left;
	    break;
	default: break;
    }
    if (info->startpara == info->lineno) {
	if (info->style == align_right ||
	    info->style == align_left ||
	    info->style == align_indent ||
	    info->style == align_center) {
	    r.left = info->bounds.left;
	    r.width = info->bounds.width;
	}
    }
    if (nextindex > (index + 1))
	index = nextindex - 1;
    if (!info->dontprint) {
	if (info->notestyle)
	    html_notestyle(info->texts, info->startindex, index, info->style,
		r, other, info->bounds.left, info->bounds.width);
	if (info->printstyle)
	    html_printtexts(info->file, info->texts, info->startline, index, info->style,
		r, other, info->bounds.left, info->bounds.width);
    }
    if (info->lastseen && info->startpage != info->lastseen->number) {
	int savedpage = info->lastseen->number;
	printpageheader(info);
	info->startpage = savedpage;
	info->lineno -= (info->startlineno - 1);
	info->paranum = 0;
    }
}

 /*
  * setstartpara
  *  remember a bunch of stuff when a beginning of paragraph is seen -
  *   the line number, index in the texts array
  *   rect of the start line, the start page
  */
static void setstartpara(struct para_info *info, struct Line *cur) {
    info->startpara = info->lineno;
    info->startindex = cur->index;
    info->startline = cur;
    info->style = align_unknown;
    if (info->lastseen)
	info->startpage = info->lastseen->number;
}

 /*
  * checkcenter
  *  looks for long centered paragraphs, and sees if it is left aligned, to make it a 
  *  blockquote (inset paragraph)
  */
static void checkcenter(struct para_info *info, struct Line *end) {
    int nlines = (info->lineno - info->startpara) + 1;
    if (info->style == align_center && nlines > 2) {
	struct Line *cur;
	int left = info->startline->r.left;
	for (cur = info->startline + 1; cur < end; cur++) {
	    if (!approx_match(left, cur->r.left, 3))
		return;
	}
	info->style = align_blockquote;
    }
}

 /*
  * calcshortlines
  *  returns a percentage of the maximum width
  */
static int calcshortlines(int width) {
    return (width * page_shortlines) / 100;
}

 /*
  * findpara
  *  called from the enumerator - page_enumlines
  *  prints out line start and end of a paragraph
  */
static int findpara(void *context, struct Line *cur, struct Line *next) {
    struct para_info *info = (struct para_info *) context;
    if (info->pagestarts) {
	struct Page *page = hashtable_get(info->pagestarts, (void *)cur->text);
	if (page) {
	    info->lastseen = page;
	    info->pageleft = page->pageleft;
	    info->dontreflow = page->dontreflow;
	    if (!info->startpara) {
		info->lineno = 1;
	    }
	    info->startlineno = info->lineno;
	    info->bounds = page_setminbounds(page_textbounds(page), info->minbounds);
	    info->shortlines = calcshortlines(info->bounds.width);
	}
    }
    if (next->text) {
	int leading;
	int have_end_spacing;
	if (!info->startpara)
	    setstartpara(info, cur);
	checkparastyle(info, cur->r, next->r.left);
	leading = (next->r.top - cur->r.top) - cur->r.height;
	have_end_spacing = ((leading > (cur->r.height/2) && (cur->r.height/2) > page_lineleading)  || leading > (page_lineleading + 2)) ? 1 : 0;
	if (have_end_spacing || atparaend(info, cur, next) ||
	    atpageend(info, cur, next)) {
	    checkcenter(info, cur);
	    printparatexts(info, cur, next, have_end_spacing);
	    page_prevstyle = info->style;
	    info->paranum++;
	    info->startpara = 0;
	    info->startindex = -1;
	    info->startline = NULL;
	}
	info->lineno++;
    } else {
        /* last line of page */
	if (!info->startpara)
	    setstartpara(info, cur);
	checkparastyle(info, cur->r, -1);
	printparatexts(info, cur, next, 1);
    }
    return 1;
}

 /*
  * page_setminbounds
  *  making sure first rect is at close to the min rect
  */
struct Rect page_setminbounds(struct Rect a, struct Rect min) {
    struct Rect r;
    if (a.width >= ((min.width * 3)/4))
	return a;
    r = rect_add(a, min);
    r.top = a.top;
    r.height = a.height;
    return r;
}

 /*
  * page_processpara
  *  prints out line numbers associated with each paragraph in a page
  */
void page_processpara(struct Page *page, struct Rect minbounds, struct array *texts, struct hashtable *pagestarts, int dontprint) {
    struct para_info info = {0};
    int i, length;
    
    removeempties(page);
    info.lineno = 1;
    info.paranum = 1;
    info.bounds = page_setminbounds(page_textbounds(page), minbounds);
    info.texts = page->texts;
    info.dontprint = dontprint;
    info.printerr = 1;
    info.notestyle = 1;
    info.shortlines = calcshortlines(info.bounds.width);
    if (!dontprint && page_showpara)
	fprintf(stderr, "page %d:\n", page->number);
    linetable_enum_linetable(page->texts, &info, 0, findpara);
    if (!texts || !pagestarts)
	return;
    length = array_length(page->texts);
    for (i = 0; i < length; i++) {
	struct Text *text = array_elementat(page->texts, i);
	array_append_element(texts, text);
	if (i == 0)
	    hashtable_put(pagestarts, (void *)text, page);
    }
}

 /*
  * page_printpara
  *  prints out line numbers associated with each paragraph in a page
  */
void page_printpara(struct Page *page, struct Rect minbounds, struct array *texts, struct hashtable *pagestarts) {
    page_processpara(page, minbounds, texts, pagestarts, 0);
}

 /*
  * page_initpara
  *  inits some stuff in a page object -- is this necessary now ???
  */
void page_initpara(struct Page *page, struct Rect minbounds) {
    page_processpara(page, minbounds, NULL, NULL, 1);
}

 /*
  * page_multipleparas
  *  enumerates all text objects and generates html, styles, and some debugging info
  */
void page_multipleparas(FILE *file, struct array *texts, struct hashtable *pagestarts, struct Rect minbounds) {
    struct para_info info = {0};
    struct para_info zero = {0};
    
    info.lineno = 1;
    info.paranum = 1;
    info.minbounds = minbounds;
    info.pagestarts = pagestarts;
    info.texts = texts;
    info.notestyle = 1;
    linetable_enum_linetable(texts, &info, 1, findpara);

    html_printstyles(file);
    if (page_showpara)
	fprintf(stderr, "\n\nconsolidated pages:\n");
    info = zero;
    info.lineno = 1;
    info.paranum = 1;
    info.minbounds = minbounds;
    info.pagestarts = pagestarts;
    info.texts = texts;
    info.printstyle = 1;
    info.printerr = 1;
    info.file = file;
    linetable_enum_linetable(texts, &info, 1, findpara);
    html_printtail(file);
}

 /*
  * page_printchapter
  *  print to stderr if page might be a chapter header
  */
void page_printchapter(struct Page *page, int fontsize) {
    int i, length = array_length(page->texts);
    if (page->number >= page_first) {
	for (i = 0; i < length; i++) {
	    struct Text *text = array_elementat(page->texts, i);
	    int size = text_fontsize(text);
	    if (size >= fontsize) {
		fprintf(stderr, "New chapter on page %d.\n", page->number);
		break;
	    }
	}
    }
}

 /*
  * page_print
  *  print to stderr paragraph grouping
  */
void page_print(struct Page *page) {
    int i, length;
    page_sort(page);
    fprintf(stderr, "<page number=\"%d\" position=\"absolute\" top=\"0\" left=\"0\" height=\"%d\" width=\"%d\">\n", page->number, page->height, page->width);
    length = array_length(page->texts);
    for (i = 0; i < length; i++) {
	struct Text *text = array_elementat(page->texts, i);
	text_print(text);
    }
    fprintf(stderr, "</page>\n");
}


struct PrintInfo {
    int lineno;
    FILE *file;
    struct array *texts;
    int length;
};

static int printline(void *context, struct Line *cur, struct Line *next) {
    struct PrintInfo *printinfo = (struct PrintInfo *) context;
    int last;
    fprintf(printinfo->file, "%2d ", printinfo->lineno);
    last = (next->index) ? next->index : printinfo->length;
    
    text_printcontents(printinfo->texts, printinfo->file, cur->index, last);
    printinfo->lineno++;
    return 1;
}

 /*
  * page_printcontents
  *  print the text lines of a page
  */
void page_printcontents(struct Page *page, FILE *file) {
    struct PrintInfo printinfo;
    page_sort(page);
    printinfo.lineno = 1;
    printinfo.file = file;
    printinfo.texts = page->texts;
    printinfo.length = array_length(page->texts);
    linetable_enum_linetable(page->texts, &printinfo, 0, printline);
}

