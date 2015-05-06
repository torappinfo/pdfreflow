/*
 * font.c 
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
#include "hashtable.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>

static struct hashtable *fonts = 0;
static struct hashtable *font_printstrs = 0;

struct Font {
    struct Atom *id;		/* id from xml */
    int size;			/* font size */
    struct Atom *family;	/* family name */
    struct Atom *color;		/* font color */
    struct Atom *printstr;	/* debugging print string, ie Times-10 */
};

 /*
  * font_free
  *  frees a font object
  */
static void font_free(struct hashtable *table, void *value, void *context) {
    struct Font *font = value;
    free(font);    
}

 /*
  * font_procs
  *  callback procs for hashtable
  */
struct hashtable_procs font_procs = {
    font_free
};

 /*
  * font_init
  *  create some static tables - fonts, font_printstrs
  */
static void font_init() {
    fonts = hashtable_special_create(0, &font_procs, NULL);
    hashtable_set_keys_are_pointers(fonts);
    font_printstrs = hashtable_create(0);
    hashtable_set_keys_are_pointers(font_printstrs);
}

 /*
  * font_get
  *  get a font object from its id
  */
static struct Font *font_get(struct Atom *id) {
    if (!fonts)
	return NULL;
    return hashtable_get(fonts, (void *)id);
}

 /*
  * font_size
  *  returns the size of a font, the id of the font is an atom
  */
int font_size(struct Atom *id) {
    struct Font *font = font_get(id);
    if (font) {
	return font->size;
    }
    return -1;
}

struct Atom *font_printstr(struct Atom *id) {
    struct Font *font = font_get(id);
    if (font) {
	return font->printstr;
    }
    return NULL;
}

 /*
  * font_family
  *  returns the family of a font, the id of the font is an atom
  */
char *font_family(struct Atom *id) {
    struct Font *font = font_get(id);
    if (font) {
	return (char *)font->family;
    }
    return NULL;
}

 /*
  * font_add
  *  Add a font to the global table
  */
void font_add(char *id, int size, char *family, char *color) {
    struct Font *font;
    char buf[100];
    if (!fonts)
	font_init();
    font = (struct Font *)malloc(sizeof(struct Font));
    font->id = attr_atom(id);
    font->size = size;
    font->family = attr_atom(family);
    font->color = attr_atom(color);
    snprintf(buf, sizeof(buf), "%s-%d", family, font->size);
    font->printstr = attr_atom(buf);
    hashtable_put(fonts, (void *) font->id, font);
    if (!hashtable_get(font_printstrs, (void *)font->printstr))
	hashtable_put(font_printstrs, (void *)font->printstr, font);
}

struct Atom *font_fromprintstr(struct Atom *printstr) {
    struct Font *font = hashtable_get(font_printstrs, (void *)printstr);
    if (!font)
	return NULL;
    return font->id;
}

 /*
  * fontcompare
  *  compare proc for qsort
  */
static int
fontcompare(const void *p1, const void *p2) {
    struct Font **s1 = (struct Font **)p1;
    struct Font **s2 = (struct Font **)p2;
    if ((*s1)->size < (*s2)->size)
	return -1;
    else if ((*s1)->size > (*s2)->size)
	return 1;
    return 0;
}

 /*
  * font_printfonts
  *  print all the fonts in the document
  */
void font_printfonts() {
    struct array *keys;
    struct hashtable *inversion;
    int i, length;
    struct array *values = 0;
    if (!fonts)
	return;
    keys = hashtable_keys(fonts, 0);
    length = array_length(keys);
    inversion = hashtable_create(0);
    for (i = 0; i < length; i++) {
	char *id = array_elementat(keys, i);
	struct Font *font = hashtable_get(fonts, id);
	void *family = (void *)font->family;
	
	values = hashtable_get(inversion, family);
	if (!values) {
	    values = array_pointer_init(0);
	    hashtable_put(inversion, family, values);
	}
	array_append_element(values, font);
    }
    array_free(keys);
    keys = hashtable_keys(inversion, 0);
    array_sortstrings(keys, 1, 1);
    length = array_length(keys);
    for (i = 0; i < length; i++) {
	char *family = array_elementat(keys, i);
	int j, vlen;
	int lastsize = -1;
	values = hashtable_get(inversion, family);
	fprintf(stderr, "%s: ", family);
	vlen = array_length(values);
	array_set_contains_pointers(values, 0);
	qsort(array_elementat(values, 0), vlen, sizeof(char *), fontcompare);
	array_set_contains_pointers(values, 1);
	for (j = 0; j < vlen; j++) {
	    struct Font *font = array_elementat(values, j);
	    if (font->size == lastsize)
		continue;
	    if (j)
		fprintf(stderr, ", ");
	    fprintf(stderr, "%d", font->size);
	    lastsize = font->size;
	}
	fprintf(stderr, "\n");
	array_free(values);
    }
    hashtable_free(inversion);
}

