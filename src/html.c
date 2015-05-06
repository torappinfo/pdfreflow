/*
 * html.c 
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
#include "attr.h"
#include "font.h"
#include "html.h"
#include "parse.h"
#include "text.h"
#include "array.h"
#include "hashtable.h"
#include "linetable.h"
#include <stdio.h>
#include <stdlib.h>

struct Style {
    struct Atom *family;	/* font family of style */
    int fontsize;		/* font size of style */
    int relsize;		/* relative size in percentage, ie 75%, 100% */
    enum para_style style;	/* pararaph style  */
    int marginleft;		/* left margin  */
    int marginright;		/* right margin  */
    int otherval;		/* usually indent (can be negative for outent)  */
    int id;			/* style count, or id, used in generating style name */
    struct FontDesc *fontdesc;	/* unique'd font */
};

struct FontDesc {
    struct Atom *family;
    int fontsize;
    int relsize;		/* relative size in percentage, ie 75%, 100% */
    int id;			/* id is the span number, ie id=5 generates span.s5 {...} */
};

static struct hashtable *styles = NULL;
static struct hashtable *fonts = NULL;
static int counters[10] = {0};
static int html_showstyle = 0;
static int html_absfontsize = 0;
static struct FontDesc *html_defaultfont = NULL;

static int styletoid[] = {
    1,
    1,
    1,
    1,
    1,
    1,
    0,
};
static char *aligns[] = {
    "left",
    "justify",
    "center",
    "right",
    "justify",
    "justify",
    "left",
};
static char *construct[] = {
    "p",
    "p",
    "p",
    "p",
    "p",
    "p",
    "blockquote",
};

static int curfontid = 0;
static int html_rag = 0;
static struct FontDesc *html_getfont(struct Atom *name);

 /*
  * html_setshowstyle
  *  print debugging info when styles are created
  */
void html_setshowstyle() {
    html_showstyle = 1;
}


 /*
  * html_setabsolute
  *  use absolute font sizes in output, rather than relative (percentage)
  */
void html_setabsolute() {
    html_absfontsize = 1;
}

 /*
  * html_setrag
  *  tells that the text is rag right
  */
void html_setrag() {
    html_rag = 1;
}

static void style_free(struct hashtable *table, void *value, void *context) {
    struct Style *style = (struct Style *) value;
    free(style);
}

static struct hashtable_procs styleprocs = {
    style_free
};

static void fontdesc_free(struct hashtable *table, void *value, void *context) {
    struct FontDesc *font = (struct FontDesc *) value;
    free(font);
}

static struct hashtable_procs fontprocs = {
    fontdesc_free
};

 /*
  * styleinit
  *  init some static hash tables - the styles and fonts of the document
  */
static void styleinit() {
    styles = hashtable_special_create(0, &styleprocs, NULL);
    fonts = hashtable_special_create(0, &fontprocs, NULL);
}

 /*
  * style_init
  *  create a style
  */
static struct Style *style_init(struct FontDesc *font, struct Atom *family, int fontsize, enum para_style styleval, int marginleft, int marginright, int otherval) {
    struct Style *style = calloc(1, sizeof(struct Style));
    int idmap = styletoid[styleval];
    
    counters[idmap]++;
    style->family = family;
    style->fontsize = fontsize;
    style->style = styleval;
    style->marginleft = marginleft;
    style->marginright = marginright;
    style->otherval = otherval;
    style->id = counters[idmap];
    style->fontdesc = font;
    return style;
}

static int html_musthave = 0;

 /*
  * html_getstyle
  *  gets a unique style
  */
static struct Style *html_getstyle(struct FontDesc *font, enum para_style styleval, struct Rect r, int otherval, int left, int width) {
    char key[200];
    struct Style *style;
    char *family = (char *)font->family;
    int fontsize = font->fontsize;
    int right = (left + width) - rect_right(r);
    int lmargin = r.left - left;
    
    if (otherval >= -1 && otherval <= 1) 
	otherval = 0;
	
    if (html_rag)
	right = 0;
    if (right < 6)
	right = 0;
    if (lmargin < 6)
	lmargin = 0;
    sprintf(key, "%s:%d:%d:%d:%d:%d", family, fontsize, (int) styleval, lmargin, right, otherval);
    style = hashtable_get(styles, key);
    if (!style) {
	if (html_showstyle) 
	    fprintf(stderr, "Style font: %s-%d, style:%s, lm:%d, rm:%d, in:%d\n", 
		family, fontsize, html_parstylestr(styleval), r.left - left,
		right, otherval);
	style = style_init(font, font->family, fontsize, styleval, lmargin, right, otherval);
	hashtable_put(styles, key, style);
    }
    return style;
}

 /*
  * fontdesc_init
  *  inits a font object used in uniquing all fonts
  */
static struct FontDesc *fontdesc_init(struct Atom *name, int fontsize) {
    struct FontDesc *font = calloc(1, sizeof(struct FontDesc));
    curfontid++;
    font->family = name;
    font->fontsize = fontsize;
    font->id = curfontid;
    return font;
}


 /*
  * html_getfont
  *  returns a font that matches name and fontsize. fonts are made unique
  */
static struct FontDesc *html_getfont(struct Atom *name) {
    char key[200];
    struct FontDesc *fontdesc;
    char *family = font_family(name);
    int fontsize = font_size(name);
    sprintf(key, "%s:%d", family, fontsize);
    if (!fonts)
	styleinit();
    fontdesc = hashtable_get(fonts, key);
    if (!fontdesc) {
	fontdesc = fontdesc_init((struct Atom *)family, fontsize);
	hashtable_put(fonts, key, fontdesc);
    }
    return fontdesc;
}

 /*
  * html_setdefaultfont
  *  sets the default font for document -- printstr format Atom, ie Times-12
  */
void html_setdefaultfont(struct Atom *defaultfont) {
    struct Atom *font = font_fromprintstr(defaultfont);
    html_defaultfont = html_getfont(font);
}

static int html_approx_match(int pos1, int pos2) {
    if ((pos2 >= (pos1 -3)) && (pos2 <= (pos1 + 3)))
	return 1;
    return 0;
}

 /*
  * html_adjustrect
  *  cleans up rect based on style
  */
struct Rect html_adjustrect(enum para_style styleval,  struct Rect r, int left, int width) {
    if (!styles)
	styleinit();
    if (html_approx_match(left, r.left)) {
	r.left = left;
    }
    if (html_approx_match(left + width, rect_right(r))) {
	r.width = (left + width) - r.left;
    }
    if (styleval == align_center) {
	r.left = left;
	r.width = width;
    }
    if (styleval == align_right) {
	r.left = left;
    }
    return r;
}

 /*
  * fontforpara
  *  finds the default font for a paragraph. If the paragraph contains the html_defaultfont, then
  *  that font is chosen. Otherwise, use the first font of the paragraph
  */
static struct FontDesc *fontforpara(struct array *texts, int startindex, int endindex) {
    struct Text *text;
    struct FontDesc *cur, *first = NULL;
    int i;
    for (i = startindex; i <= endindex; i++) {
	text = array_elementat(texts, i);
	cur = html_getfont(text_font(text));
	if (i == startindex)
	    first = cur;
	if (cur == html_defaultfont)
	    return cur;
    }
    return first;
}


 /*
  * html_notestyle
  *  records a style, which is uniqued
  */
void html_notestyle(struct array *texts, int startindex, int endindex, enum para_style styleval,  struct Rect r, int otherval, int left, int width) {
    struct Style *style;
    struct Text *text;
    struct FontDesc *font = fontforpara(texts, startindex, endindex);
    int i;
    
    r = html_adjustrect(styleval, r, left, width);
    style = html_getstyle(font, styleval, r, otherval, left, width);
    for (i = startindex; i <= endindex; i++) {
	text = array_elementat(texts, i);
	html_getfont(text_font(text));
    }
}

 /*
  * html_printtext
  *  prints a single text object
  */
static void html_printtext(FILE *file, struct Style *style, struct Text *text, int first, int last, int oneliner, int needs_break, struct Text *next) {
    struct FontDesc *fontdesc = html_getfont(text_font(text));
    int needspan = (fontdesc == style->fontdesc) ? 0 : 1;
    if (needspan) 
	fprintf(file, "<span class=\"s%d\">", fontdesc->id);
    text_printtext(text, next, first, file);
    if (needspan) 
	fprintf(file, "</span>");
    if (style->style == align_center || style->style == align_unknown) {
	if (!oneliner && !last && needs_break)
	    fprintf(file, "<br>");
    }
    if (!last)
	fprintf(file, "\n");
}

 /*
  * html_printtexts
  *  prints the paragraph text
  */
void html_printtexts(FILE *file, struct array *texts, struct Line *startline, int endindex, enum para_style styleval,  struct Rect r, int otherval, int left, int width) {
    struct Style *style;
    char *element;
    int startindex = startline->index;
    struct Text *text;
    struct FontDesc *font = fontforpara(texts, startindex, endindex);
    int i;
    
    html_musthave = 1;
    r = html_adjustrect(styleval, r, left, width);
    style = html_getstyle(font, styleval, r, otherval, left, width);
    element = construct[style->style];
    fprintf(file, "<%s class=\"%c%d\">", element, element[0], style->id);
    for (i = startindex; i <= endindex; i++) {
	int needs_break = (i == (startline[1].index - 1));
	text = array_elementat(texts, i);
	html_printtext(file, style, text, i == startindex, i == endindex, startindex == endindex, needs_break, array_elementat(texts, i+1));
	if (needs_break)
	    startline++;
    }
    fprintf(file, "</%s>\n", element);
}

 /*
  * stylecompare
  *  qsort comparison proc
  */
static int stylecompare(const void *p1, const void *p2) {
    struct Style **s1 = (struct Style **)p1;
    struct Style **s2 = (struct Style **)p2;
    int id1, id2;
    id1 = (*s1)->id + (styletoid[(*s1)->style] * 100);
    id2 = (*s2)->id + (styletoid[(*s2)->style] * 100);
    if (id1 < id2)
	return -1;
    if (id1 > id2)
	return 1;
    return 0;
}

 /*
  * fontcompare
  *  qsort comparison proc
  */
static int fontcompare(const void *p1, const void *p2) {
    struct FontDesc **s1 = (struct FontDesc **)p1;
    struct FontDesc **s2 = (struct FontDesc **)p2;
    if ((*s1)->id < (*s2)->id)
	return -1;
    if ((*s1)->id > (*s2)->id)
	return 1;
    return 0;
}

 /*
  * html_getlist
  *  return a sorted list of keys from a hashtable
  */
struct array *html_getlist(struct hashtable *hash, int (*proc)(const void *p1, const void *p2)) {
    struct array *keys = hashtable_keys(hash, 0);
    struct array *styledescs = array_pointer_init(0);
    int i, length = array_length(keys);
    for (i = 0; i < length; i++) {
	char *key = array_elementat(keys, i);
	struct Style *style = hashtable_get(hash, key);
	array_append_element(styledescs, style);
    }
    length = array_length(styledescs);
    array_set_contains_pointers(styledescs, 0);
    qsort(array_elementat(styledescs, 0), length, sizeof(char *), proc);
    array_set_contains_pointers(styledescs, 1);
    array_free(keys);
    return styledescs;
}

 /*
  * html_getfontlist
  *  return a list of all the fonts
  */
struct array *html_getfontlist() {
    return html_getlist(fonts, fontcompare);
}

 /*
  * html_getstylelist
  *  return a list of all the styles
  */
struct array *html_getstylelist() {
    return html_getlist(styles, stylecompare);
}

 /*
  * html_printstyle
  *  prints a paragraph class
  */
static void html_printstyle(FILE *file, struct Style *style) {
    char *element = construct[style->style];
    fprintf(file, "%s.%c%d {\n", element, element[0], style->id);
    if (style->marginleft)
	fprintf(file, "    margin-left: %dpx;\n", style->marginleft);
    if (!html_rag && style->marginright)
	fprintf(file, "    margin-right: %dpx;\n", style->marginright);
    fprintf(file, "    text-align: %s;\n", aligns[style->style]);
    if (style->style == align_indent || style->style == align_outdent)
	fprintf(file, "    text-indent: %dpx;\n", style->otherval);
    if (style->fontdesc != html_defaultfont) {
	if (style->family != html_defaultfont->family)
	    fprintf(file, "    font-family: %s;\n", (char *)style->family);
	if (style->fontsize != html_defaultfont->fontsize) {
	    if (html_absfontsize)
		fprintf(file, "    font-size: %dpx;\n", style->fontsize);
	    else
		fprintf(file, "    font-size: %d%s;\n", style->relsize, "%");
	}
    }
    fprintf(file, "}\n\n");
}

 /*
  * html_printfontdesc
  *  prints a span class
  */
static void html_printfontdesc(FILE *file, struct FontDesc *fontdesc) {
    fprintf(file, "span.s%d {\n", fontdesc->id);
    fprintf(file, "    font-family: %s;\n", (char *)fontdesc->family);
    if (html_absfontsize)
	fprintf(file, "    font-size: %dpx;\n", fontdesc->fontsize);
    else
	fprintf(file, "    font-size: %d%s;\n", fontdesc->relsize, "%");
    fprintf(file, "}\n\n");
}

 /*
  * printintro
  *  prints the beginning of the html file
  */
static void printintro(FILE *file) {
    fprintf(file, "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    fprintf(file, "<html>\n");
    fprintf(file, "<head>\n");
    fprintf(file, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", parse_encoding());
}

 /*
  * printbodystyle
  *  print body css entry if necessary
  */
static void printbodystyle(FILE *file) {
    if (html_absfontsize) {
	fprintf(file, "body {\n");
	fprintf(file, "    font-family: %s;\n", (char *)html_defaultfont->family);
	fprintf(file, "    font-size: %dpx;\n", html_defaultfont->fontsize);
	fprintf(file, "}\n\n");
    }
}

static int mods[] = {0, 20, 25, 33, 40, 50, 60, 67, 75, 80, 100};
 /*
  * calcrelsize
  *  calculate the percentage relative to the default font size
  */
static int realcalcrelsize(int fontsize) {
    if (fontsize == html_defaultfont->fontsize)
	return 100;
    else {
	int relsize = (fontsize * 100) / html_defaultfont->fontsize;
	int div = relsize / 100;
	int mod = relsize % 100;
	int *cur, d1, d2;
	
	if (mod == 0)
	    return relsize;
	for (cur = mods; *cur < mod; cur++);
	if (mod == *cur)
	    return relsize;
	d1 = mod - cur[-1];
	d2 = cur[0] - mod;
	if (d1 < d2) 
	    return (div * 100) + cur[-1];
	else
	    return (div *100) + cur[0];
    }
}

static int calcrelsize(int fontsize) {
    int relsize = realcalcrelsize(fontsize);
    if (relsize < 75)
	relsize = 75;
    if (relsize > 300)
	relsize = 300;
    return relsize;
}


 /*
  * html_printstyles
  *  enumerate styles for css tag in head section
  */
void html_printstyles(FILE *file) {
    struct array *stylelist = html_getstylelist();
    struct array *fontlist = html_getfontlist();
    int i, length;
    printintro(file);
    fprintf(file, "<style type=\"text/css\">\n");
    printbodystyle(file);
    length = array_length(fontlist);
    length = array_length(stylelist);
    for (i = 0; i < length; i++) {
	struct Style *style = array_elementat(stylelist, i);
	style->relsize = calcrelsize(style->fontsize);
	html_printstyle(file, style);
    }
    length = array_length(fontlist);
    for (i = 0; i < length; i++) {
	struct FontDesc *fontdesc = array_elementat(fontlist, i);
	if (fontdesc != html_defaultfont) {
	    fontdesc->relsize = calcrelsize(fontdesc->fontsize);
	    html_printfontdesc(file, fontdesc);
	}
    }
    fprintf(file, "</style>\n");
    fprintf(file, "</head>\n");
    fprintf(file, "</body>\n");
    array_free(stylelist);
    array_free(fontlist);
}

 /*
  * html_printtail
  *  prints the end of the html file
  */
void html_printtail(FILE *file) {
    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");
}

 /*
  * parstylestr
  *  get print string for para_style
  */
char *html_parstylestr(enum para_style style) {
    char *result = "";
    switch(style) {
	case align_unknown:
	    result = "unknown";
	    break;
	case align_left:
	    result = "left";
	    break;
	case align_center:
	    result = "center";
	    break;
	case align_right:
	    result = "right";
	    break;
	case align_indent:
	    result = "indent";
	    break;
	case align_outdent:
	    result = "outdent";
	    break;
	case align_blockquote:
	    result = "blockquote";
	    break;
    }
    return result;
}


