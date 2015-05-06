/*
 * linetable.c 
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

#include "linetable.h"
#include "array.h"
#include "text.h"
#include <stdio.h>

 /*
  * enumlines
  *  enumerates each unique line of the texts, that is, if multple texts are on the
  *  same line, only the first text of the line is enumerated, the rest of the line is
  *  skipped.
  */
void linetable_enumlines(struct array *texts, void *context, int (*proc)(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex)) {
    int i, length;
    struct Text *cur = NULL, *next = NULL;
    int last, ret, curindex = -1, nextindex = -1;
    struct Rect rect;
    
    length = array_length(texts);
    last = length - 1;
    for (i = 0; i < length; i++) {
	if (!cur) {
	    cur = array_elementat(texts, i);
	    curindex = i;
	    rect = text_bounds(cur);
	}
	if (!next && (i != last)) {
	    next = array_elementat(texts, i+1);
	    nextindex = i+1;
	}
	if (next && text_intersects(cur, next)) {
	    rect = rect_add(rect, text_bounds(next));
	    next = NULL;
	    nextindex = -1;
	    continue;
	}
	ret = proc(context, cur, next, rect, curindex, nextindex);
	if (!ret)
	    break;
	cur = next;
	curindex = nextindex;
	if (cur)
	    rect = text_bounds(cur);
	else
	    rect = rect_empty;
	next = NULL;
	nextindex = -1;
    }
}

static struct array *lines = NULL;

 /*
  * saveline
  *  add the current Text, index, and rect to the lines array
  */
static int saveline(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex) {
    struct Line line;
    line.text = cur;
    line.index = index;
    line.r = rect;
    array_append_element(lines, &line);
    return 1;
}

 /*
  * addempty
  *  add an empty Line struct at the end of the lines array.
  */
static void addempty() {
    struct Line line = {0};
    int length = array_length(lines);
    array_append_element(lines, &line);
    array_setlength(lines, length);
}


 /*
  * linetable_enum_linetable
  *  enumerate the lines in the XML document.
  *  if save is 1, then remember the line table for the next iteration. 
  *  if save is 1, and there is a previous line table, use it.
  *  if save is 0, then empty line table.
  *  proc gets a context pointer, the current and next Line structs.
  *  if proc returns 0, end enumeration
  */
void linetable_enum_linetable(struct array *texts, void *context, int save, int (*proc)(void *context, struct Line *cur, struct Line *next)) {
    int i, length, last, ret;
    struct Line *cur = NULL, *next = NULL;
    struct Line lastline = {0};
    if (!lines)
	lines = array_init(sizeof(struct Line), 0);
    length = array_length(lines);
    if (!length) {
	linetable_enumlines(texts, NULL, saveline);
	addempty();
    }
    length = array_length(lines);
    last = length - 1;
    lastline.index = array_length(texts);
    for (i = 0; i < length; i++) {
	if (!cur)
	    cur = array_elementat(lines, i);
	if (!next) {
	    if (i < last)
		next = array_elementat(lines, i+1);
	    else
		next = &lastline;
	}
	ret = proc(context, cur, next);
	cur = next;
	next = NULL;
	if (!ret)
	    break;
    }
    if (!save)
	array_setlength(lines, 0);
}


