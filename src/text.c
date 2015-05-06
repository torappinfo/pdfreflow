/*
 * text.c 
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
#include "rect.h"
#include "text.h"
#include "font.h"
#include "array.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct Text {
    struct Rect r;  	/* bounding rect of text */
    int baseline;	/* baseline of this text */
    struct Atom *font;	/* atom that refers to font id */
    char *buf;		/* contents of text tag        */
    char *other;	/* pointer to new text if buf had part of a hyphenated string */
};

 /*
  * text_init
  *  creates a new text object, calcs baseline
  */
struct Text *text_init(int top, int left, int width, int height, struct Atom *font, char *buf) {
    struct Text *text = calloc(1, sizeof(struct Text));
    if (text) {
	int size;
	text->r.top = top;
	text->r.left = left;
	text->r.width = width;
	text->r.height = height;
	text->font = font;
	text->buf = strdup(buf);
	size = font_size(font);
	text->baseline =  (size > 0)  ? (top + size) : top;
    }
    return text;
}


 /*
  * text_free
  *  frees text object
  */
void text_free(struct Text *text) {
    free(text->buf);
    free(text);
}


 /*
  * text_isempty
  *  returns 1 if text buf is only whitespace
  */
int text_isempty(struct Text *text) {
    unsigned char *cur;
    for (cur = (unsigned char *)text->buf; *cur; cur++) {
	if (!isspace(*cur)) {
	    if (*cur == '<') {
		for (cur++; *cur && *cur != '>' ; cur++);
		if (!cur)
		    return 1;
		continue;
	    }
	    return 0;
	}
    }
    return 1;
}

 /*
  * text_numericvalue
  *  return numeric value of text buffer
  *  if no number, or there is other text besides a number, other than white space
  *  -1 is returned
  */
int text_numericvalue(struct Text *text) {
    unsigned char *cur;
    for (cur = (unsigned char *)text->buf; *cur; cur++) {
	if (!isspace(*cur))
	    break;
    }
    if (!*cur)
	return -1;
    if (*cur > '0' && *cur <= '9') {
	char *end;
	int val = strtol((char *)cur, &end, 10);
	if (!*end || (*end == ' ') || (*end == '<'))
	    return val;
	for (cur = (unsigned char *)end; *cur; cur++) {
	    if (!isspace(*cur))
		break;
	}
	return (*cur) ? -1 : val;
    }
    return -1;
}

 /*
  * text_length
  *  returns the length of the contained text
  */
int text_length(struct Text *text) {
    return text->buf ? strlen(text->buf) : 0;
}

 /*
  * text_width
  *  returns the width of the contained text
  */
int text_width(struct Text *text) {
    return text->r.width;
}

 /*
  * text_left
  *  returns the left of the contained text
  */
int text_left(struct Text *text) {
    return text->r.left;
}

 /*
  * text_right
  *  returns the right of the contained text
  */
int text_right(struct Text *text) {
    return rect_right(text->r);
}

 /*
  * text_baseline
  *  returns the baseline of the contained text (0 is top of page, increases going down)
  */
int text_baseline(struct Text *text) {
    return text->baseline;
}

 /*
  * text_height
  *  returns the height of the contained text (0 is top of page, increases going down)
  */
int text_height(struct Text *text) {
    return text->r.height;
}

 /*
  * text_top
  *  returns the height of the contained text (0 is top of page, increases going down)
  */
int text_top(struct Text *text) {
    return text->r.top;
}

 /*
  * text_leading
  *  returns the height of the contained text (0 is top of page, increases going down)
  */
int text_leading(struct Text *text,  struct Text *other) {
    int height = text_bounds(text).height;
    return (other->r.top - text->r.top) - height;
}

 /*
  * text_font
  *  returns the font of the contained text
  */
struct Atom *text_font(struct Text *text) {
    return text->font;
}

 /*
  * text_fontprintstr
  *  returns a print string with font-fontsize, ie Times-10
  */
struct Atom *text_fontprintstr(struct Text *text) {
    return font_printstr(text->font);
}


 /*
  * text_fontsize
  *  returns the font of the contained text
  */
int text_fontsize(struct Text *text) {
    return font_size(text->font);
}

 /*
  * intersectsY
  *  returns 1 if the bounding rects intersect vertically, 0 otherwise
  */
static int intersectsY(struct Text *text, struct Text *other) {
    int mybot = text->r.top + text_fontsize(text);
    int otherbot = other->r.top + text_fontsize(other);
    
    if (mybot <= other->r.top)
	return 0;
    if (text->r.top >= otherbot)
	return 0;
    return 1;
}

static int text_approx_match(int pos1, int pos2, int delta) {
    if ((pos2 >= (pos1 - delta)) && (pos2 <= (pos1 + delta)))
	return 1;
    return 0;
}
 /*
  * text_intersects
  *  returns 1 if the 2 text objects are on the same line
  */
int text_intersects(struct Text *text, struct Text *other) {
    if (text_approx_match(text->baseline, other->baseline, 2))
	return 1;
    return (intersectsY(text, other) && !rect_intersects(text->r, other->r));
}

 /*
  * text_compare
  *  called by qsort to compare text objects, top to bottom, left to right
  */
int text_compare(struct Text *text, struct Text *other) {
    if (text_intersects(text, other)) {
	if (text->r.left < other->r.left)
	    return -1;
	if (text->r.left > other->r.left)
	    return 1;
	return 0;
    }
    
    if (text->r.top < other->r.top)
	return -1;
    if (text->r.top > other->r.top)
	return 1;
    if (text->r.left < other->r.left)
	return -1;
    if (text->r.left > other->r.left)
	return 1;
    return 0;
}
 /*
  * text_compare
  *  returns -1 if this is before other in page
  *  return 1 if this is after other in page
  *  return 0 if the same position
  *  order is first top to bottom, and then for texts on the same line,
  *  left to right.
  */
 /*
int text_compare(struct Text *text, struct Text *other) {
    if (text->r.top < other->r.top)
	return -1;
    if (text->r.top > other->r.top)
	return 1;
    if (text->r.left < other->r.left)
	return -1;
    if (text->r.left > other->r.left)
	return 1;
    return 0;
}
  */

 /*
  * text_rect
  *  returns the rect of the contained text
  */
struct Rect text_rect(struct Text *text) {
    return text->r;
}

 /*
  * text_bounds
  *  returns the rect of the contained text
  */
struct Rect text_bounds(struct Text *text) {
    struct Rect r = text->r;
    int size = font_size(text->font);
    if (size > r.height)
	r.height = size;
    return r;
}

 /*
  * text_print
  *  print the contents of text
  */
void text_print(struct Text *text) {
    fprintf(stderr, "<text top=\"%d\" left=\"%d\" width=\"%d\" height=\"%d\" font=\"%d\">%s</text>\n", text->r.top, text->r.left, text->r.width, text->r.height, font_size(text->font), text->buf);
}

static char *quo = "&quot;";
int quolen = 0;
static char *ldquo = "&ldquo;";
int ldquolen = 0;
static char *rdquo = "&rdquo;";
int rdquolen = 0;
static char *luquo = "“";
int luquolen = 0;
static char *ruquo = "”";
int ruquolen = 0;
static char *d1 = "–";
int d1len = 0;
static char *d2 = "—";
int d2len = 0;

static void initquotes() {
    quolen = strlen(quo);
    ldquolen = strlen(ldquo);
    rdquolen = strlen(rdquo);
    luquolen = strlen(luquo);
    ruquolen = strlen(ruquo);
    d1len = strlen(d1);
    d2len = strlen(d2);
}

 /*
  * text_startswithquote
  *  returns 1 if text start with a capital letter or a digit
  */
int text_startswithquote(struct Text *text) {
    int i, length = strlen(text->buf);
    unsigned char ch;
    if (!quolen) 
	initquotes();
    for (i = 0; i < length; i++) {
	ch = (unsigned char)text->buf[i];
	if (ch == '<') {
	    for (i++; i < length && text->buf[i] != '>'; i++);
	    if (i == length)
		return 0;
	    continue;
	}
	if (islower(ch) || isspace(ch))
	    return 0;
	if (ch == '"' || !strncmp(text->buf + i, quo, quolen) || !strncmp(text->buf + i, ldquo, ldquolen) || !strncmp(text->buf + i, luquo, luquolen) )
	    return 1;
	return 0;
    }
    return 0;
}
 /*
  * text_startswithcap
  *  returns 1 if text start with a capital letter or a digit
  */
int text_startswithcap(struct Text *text) {
    int i, length = strlen(text->buf);
    unsigned char ch;
    if (!quolen) 
	initquotes();
    for (i = 0; i < length; i++) {
	ch = (unsigned char)text->buf[i];
	if (ch == '<') {
	    for (i++; i < length && text->buf[i] != '>'; i++);
	    if (i == length)
		return 0;
	    continue;
	}
	if (islower(ch) || isspace(ch))
	    return 0;
	if (isupper(ch) || isdigit(ch) || ch == '"' || !strncmp(text->buf + i, quo, quolen) || !strncmp(text->buf + i, ldquo, ldquolen) || !strncmp(text->buf + i, luquo, luquolen) )
	    return 1;
	return 0;
    }
    return 0;
}

 /*
  * text_endsinpunc
  *  returns 1 if text ends in punctuation (.!?) character
  */
int text_endsinpunc(struct Text *text) {
    int length = strlen(text->buf);
    int i;
    unsigned char ch;
    if (!quolen) 
	initquotes();
    for (i = length - 1; i >= 0; i--) {
	ch = (unsigned char)text->buf[i];
	if (ch == '.' || ch == '?' || ch == '!' || ch == ':')
	    return 1;
	if (ch == ';') {
	    if (i >= (quolen - 1)) {
		if (!strncmp(text->buf + (i - quolen) + 1, quo, quolen)) {
		    i -= (quolen - 1);
		    continue;
		}
	    }
	    if (i >= (rdquolen - 1)) {
		if (!strncmp(text->buf + (i - rdquolen) + 1, rdquo, rdquolen)) {
		    i -= (rdquolen - 1);
		    continue;
		}
	    }
	    return 0;
	}
	if (ch == '-') {
	    if (!i)
		return 0;
	    ch = text->buf[i-1];
	    if (isalpha(ch))
		return 0;
	    return 1;
	}
	if (isspace(ch))
	    continue;
	if (ch == '>') {
	    for (i--; i >= 0 && text->buf[i] != '<'; i--);
	    if (i == 0)
		return 0;
	    continue;
	}
	if (i >= (ruquolen - 1)) {
	    if (!strncmp(text->buf + (i - ruquolen) + 1, ruquo, ruquolen)) {
		i -= (ruquolen - 1);
		continue;
	    }
	}
	if (i >= (d1len - 1)) {
	    if (!strncmp(text->buf + (i - d1len) + 1, d1, d1len)) {
		i -= (d1len - 1);
		return 1;
	    }
	}
	if (i >= (d2len - 1)) {
	    if (!strncmp(text->buf + (i - d2len) + 1, d2, d2len)) {
		i -= (d2len - 1);
		continue;
	    }
	}
	return 0;
    }
    return 0;
}

 /*
  * text_endsinhyphen
  *  returns 1 if text ends in punctuation (.!?) character
  */
static char *text_endsinhyphen(struct Text *text) {
    int length;
    int i;
    unsigned char ch;
    char *buf, *tag;
    
    buf = (text->other) ? text->other : text->buf;
    length = strlen(buf);
    for (i = length - 1; i >= 0; i--) {
	ch = (unsigned char)buf[i];
	if (ch == '-') {
	    if (i == 0)
		return NULL;
	    ch = buf[i - 1];
	    if (!isalpha(ch) || isupper(ch))
		return NULL;
	    tag = strchr(buf + i, '<');
	    if (tag)
		memmove(buf+i, tag, strlen(tag) + 1);
	    else
		buf[i] = 0;
	    return buf + i;
	}
	if (isspace(ch))
	    continue;
	if (ch == '>') {
	    for (i--; i >= 0 && buf[i] != '<'; i--);
	    if (i == 0)
		return NULL;
	    continue;
	}
	return NULL;
    }
    return NULL;
}
/*
static char *text_endsinhyphen(struct Text *text) {
    int length;
    int i;
    char ch;
    char *buf;
    
    buf = (text->other) ? text->other : text->buf;
    length = strlen(buf);
    for (i = length - 1; i >= 0; i--) {
	ch = buf[i];
	if (ch == '-') {
	    if (i == 0)
		return NULL;
	    ch = buf[i - 1];
	    if (!isalpha(ch) || isupper(ch))
		return NULL;
	    return buf + i;
	}
	if (isspace(ch))
	    continue;
	if (ch == '>') {
	    for (i--; i >= 0 && buf[i] != '<'; i--);
	    if (i == 0)
		return NULL;
	    continue;
	}
	return NULL;
    }
    return NULL;
}
*/

static char *skipfirsttag(char *buf) { return buf; }
/*
static char *skipfirsttag(char *buf) {
    int i, length = strlen(buf);
    char ch;
    for (i = 0; i < length; i++) {
	ch = buf[i];
	if (ch == '<') {
	    for (i++; i < length && buf[i] != '>'; i++);
	    if (i == length)
		return buf + length;
	    continue;
	}
	return buf + i;
    }
    return buf + length;
}
*/

 /*
  * text_hyphencont
  *  returns the continuation of the hypenated word on previous line
  *  also sets othere field, so that the continuation isn't printed twice
  */
static char *text_hyphencont(struct Text *text) {
    char *space;
    if (text->other)
	return skipfirsttag(text->buf);
    space = strchr(text->buf, ' ');
    if (space) {
	*space = 0;
	text->other = space + 1;
    } else {
	text->other = text->buf + strlen(text->buf);
    }
    return skipfirsttag(text->buf);
}

 /*
  * printstr
  *  prints the contents of the str, but also looks for leading spaces to be replace with 
  *  non-blocking spaces -- some PDFs do indentation this way, a bit of a hack.
  */
static int printstr(FILE *file, char *str, int isparastart) {
    if (isparastart) {
	while (*str == ' ') {
	    fprintf(file, "&nbsp;");
	    str++;
	}
    }
    return fprintf(file, "%s", str);
}


 /*
  * text_printtext
  *  prints contents of text object, taking into account hyphenation.
  */
int text_printtext(struct Text *text, struct Text *next, int isparastart, FILE *outfile) {
    char *hyphen = text_endsinhyphen(text);
    if (hyphen && next && !text_startswithcap(next)) {
	printstr(outfile, text->other ? text->other: text->buf, isparastart);
	return fprintf(outfile, "%s", text_hyphencont(next));
    }
    return printstr(outfile, text->other ? text->other: text->buf, isparastart);
}


 /*
  * text_printcontents
  *  prints the contents of text objects into files
  */
void text_printcontents(struct array *texts, FILE *file, int start, int last) {
    int i;
    for (i = start; i < last; i++) {
	struct Text *text = array_elementat(texts, i);
	fprintf(file, "%s", text->buf);
    }
    fprintf(file, "\n");
}

