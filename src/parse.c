/*
 * parse.c 
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
#include "page.h"
#include "parse.h"
#include "text.h"
#include "array.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define PDF2XML "<pdf2xml"
#define BUFSIZE 102400

static char errbuf[1024];
static int lineno = 0;
static char *curtag = 0;
static char *curval = 0;
static char *encoding = 0;

struct array *pages = 0;

struct Atom *atom_encoding;
struct Atom *atom_number;
struct Atom *atom_position;
struct Atom *atom_top;
struct Atom *atom_left;
struct Atom *atom_height;
struct Atom *atom_width;
struct Atom *atom_font;
struct Atom *atom_id;
struct Atom *atom_size;
struct Atom *atom_family;
struct Atom *atom_color;

static void parse_init() {
    pages = array_pointer_init(0);
    atom_encoding = attr_atom("encoding");
    atom_number = attr_atom("number");
    atom_position = attr_atom("position");
    atom_top = attr_atom("top");
    atom_left = attr_atom("left");
    atom_height = attr_atom("height");
    atom_width = attr_atom("width");
    atom_font = attr_atom("font");
    atom_id = attr_atom("id");
    atom_size = attr_atom("size");
    atom_family = attr_atom("family");
    atom_color = attr_atom("color");
}

 /*
  * parse_encoding
  *  returns the character encoding
  */
char *parse_encoding() {
    return encoding;
}


 /*
  * mygets
  *  version of fgets that maintains the current line number of the input
  */
static char *
mygets(char *s, int n, FILE *stream) {
    char *res = fgets(s, n, stream);
    lineno++;
    return res;
}

 /*
  * skipwhite
  *  skip over white spaces using isspace.
  */
static char *skipwhite(char *buf) {
    char *cur;
    for (cur = buf; *cur; cur++) {
	if (!isspace(*cur))
	    return cur;
    }
    return cur;
}

 /*
  * skiptoken
  *  skip over non spaces && non endchar.
  */
static char *skiptoken(char *buf, char endchar) {
    char *cur;
    for (cur = buf; *cur; cur++) {
	if (isspace(*cur) || *cur == endchar)
	    return cur;
    }
    return cur;
}

static void parse_errout(char *str) {
    fprintf(stderr, "%s at line %d.\n", str, lineno);
}


 /*
  * parseattr
  *  parses a key/value attribute pair, ie name="Fred Smith"
  *  *err is set to 1 if a parse error occured
  *  returns the current position in buffer
  */
static char *parseattr(char *cur, int *err) {
    char *name = NULL;
    char *value = NULL;
    cur = skipwhite(cur);
    name = cur;
    cur = skiptoken(cur, '=');
    if (isspace(*cur)) {
	*cur = 0;
	cur = skipwhite(cur + 1);
    }
    if (*cur != '=') {
	parse_errout("Missing '=' at");
	goto nope;
    }
    *cur = 0;
    cur = skipwhite(cur + 1);
    if (*cur != '"') {
	parse_errout("Missing start quote");
	goto nope;
    }
    cur++;
    value = cur;
    cur = strchr(cur, '"');
    if (*cur != '"') {
	parse_errout("Missing end quote");
	goto nope;
    }
    *cur = 0;
    attr_put(name, value);
    cur = cur + 1;
    return cur;
nope:
    *err = 1;
    return cur + strlen(cur);
}

 /*
  * parsetag
  *  parses xml tag. endtag is the ending tag, ie </page>
  *  return 1 on success, 0 on error
  *  key value attributes are store in attr.h
  *  curtag is tagname, curval is value between tag & endtag
  */
static int parsetag(char *cur, char *endtag) {
    int err = 0;
    attr_reset();
    curtag = cur + 1;
    curval = NULL;
    cur = skiptoken(cur, '>');
    if (*cur == '>') {
	*cur = 0;
	return 1;
    }
    if (isspace(*cur)) {
	*cur++ = 0;
    }
    while (!err && *cur && (*cur != '>') && (*cur != '/') && (*cur != '?')) {
	cur = parseattr(cur, &err);
    }
    if (err)
	return 0;
    if (*cur == '>' && endtag) {
	cur++;
	curval = cur;
	cur = strstr(cur, endtag);
	if (cur) {
	    *cur = 0;
	}
    }
    return 1;
}

 /*
  * parse_pdf2xml
  *  parses a pdf2xml text file
  */
static void parsexmltag(char *buf) {
    if (parsetag(buf, NULL)) {
	char *val = attr_get(atom_encoding);
	if (val)
	    encoding = strdup(val);
    }

}

 /*
  * getint
  *  returns -1 on error
  */
static int getint(struct Atom *atom) {
    char *valstr = attr_get(atom);
    char *end = 0;
    int val;
    if (!valstr)
	return -1;
    val = strtol(valstr, &end, 10);
    if (end != valstr && !*end) {
	return val;
    }
    fprintf(stderr, "Attribute %s has an invalid value %s on line %d.\n", 
	(char *)atom, valstr, lineno);
    return -1;
}

 /*
  * parse_text
  *  parses a text entry
  *  return 1 on success, NULL on error
  */
static struct Text *parse_text(FILE *file, char *buf, char *cur) {
    struct Text *text = NULL;
    if (parsetag(cur, "</text>")) {
	int top = getint(atom_top);
	int left = getint(atom_left);
	int height = getint(atom_height);
	int width = getint(atom_width);
	char *fontid = attr_get(atom_font);
	struct Atom *font = NULL;
	if (!fontid) {
	    parse_errout("Missing font id value");
	} else {
	    font = attr_atom(fontid);
	    if (!font_family(font)) {
		sprintf(errbuf, "Invalid font id %s", fontid);
		parse_errout(errbuf);
	    }
	}
	text = text_init(top, left, width, height, font, curval);
    }
    return text;
}

 /*
  * parse_font
  *  parses a page entry
  *  return 1 on success, 0 on error
  */
static int parse_font(FILE *file, char *buf, char *cur) {
    if (parsetag(cur, NULL)) {
	int size = getint(atom_size);
	char *id = attr_get(atom_id);
	char *family = attr_get(atom_family);
	char *color = attr_get(atom_color);
	font_add(id, size, family, color);
	return 1;
    }
    return 0;
}

#define PAGE_END "</page>"

 /*
  * parse_page
  *  parses a page entry
  *  return 1 on success, 0 on error
  */
static struct Page *parse_page(FILE *file, char *buf, char *cur) {
    static int pageend = 0;
    struct Page *page = NULL;
    if (!pageend)
	pageend = strlen(PAGE_END);
    if (parsetag(cur, NULL)) {
	int number = getint(atom_number);
	int width = getint(atom_width);
	int height = getint(atom_height);
	struct Page *page = page_init(number, width, height);
	while (mygets(buf, BUFSIZE, file)) {
	    cur = skipwhite(buf);
	    if (!strncmp(cur, "<text", 5)) {
		struct Text *text = parse_text(file, buf, cur);
		if (text)
		    page_addtext(page, text);

	    } else if (!strncmp(cur, "<fontspec", 9)) {
		if (!parse_font(file, buf, cur)) {

		}
	    } else if (!strncmp(cur, PAGE_END, pageend)) {
		return page;
	    }
	}
    }
    return page;
}

static void freepage(struct array *vec, void **element, void *context) {
    struct Page **page = (struct Page **)element;
    page_free(*page);
}

struct array_procs procs = {
    NULL,
    NULL,
    freepage,
    NULL
};

#define PAGE "<page"
#define PDF2XML_END "</pdf2xml>"
 /*
  * parsepdf2xml
  *  parses a pdf2xml text file
  */
struct array *parsepdf2xml(FILE *file, char *buf) {
    char *cur;
    int endlen = strlen(PDF2XML_END);
    struct array *pages = array_special_init(&procs, 0, NULL);
    array_set_contains_pointers(pages, 1);
    while (mygets(buf, BUFSIZE, file)) {
	cur = skipwhite(buf);
	if (!strncmp(cur, "<page", 5)) {
	    struct Page *page = parse_page(file, buf, cur);
	    if (!page) {
		
	    } else {
		array_append_element(pages, page);
	    }
	} else if (!strncmp(cur, PDF2XML_END, endlen)) {
	    return pages;
	}
    }
    return pages;
}

 /*
  * parse_pdf2xml
  *  parses a pdf2xml text file
  *  returns a array of Pages
  */
struct array *parse_pdf2xml(FILE *file) {
    char *buf = malloc(BUFSIZE);
    int xmllen = strlen(PDF2XML);
    char *ret, *cur;
    struct array *pages = NULL;
    
    parse_init();
    
    while ((ret = mygets(buf, BUFSIZE, file))) {
	cur = skipwhite(buf);
	if (!strncmp(cur, "<?xml", 5)) {
	    parsexmltag(cur);
	} else if (!strncmp(cur,"<!DOCTYPE", 9)) {
	
	} else if (!strncmp(cur, PDF2XML, xmllen)) {
	    pages = parsepdf2xml(file, buf);
	    break;
	} else if (*cur == '\n' || !*cur) {
	
	} else {
	    fprintf(stderr, "Unknown input at line %d: %s", lineno, buf);
	}
    }
    free(buf);
    return pages;
}


