/*
 * text.h 
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

#ifndef INCLUDED_TEXT_H
#define INCLUDED_TEXT_H
struct Text;
struct Atom;

#include "rect.h"
#include <stdio.h>

struct Text *text_init(int top, int left, int width, int height, struct Atom *font, char *buf);
void text_free(struct Text *text);

 /*
  * text_isempty
  *  returns 1 if text buf is only whitespace
  */
int text_isempty(struct Text *text);
 /*
  * text_numericvalue
  *  return numeric value of text buffer
  *  if no number, or there is other text besides a number, other than white space
  *  -1 is returned
  */
int text_numericvalue(struct Text *text);
 /*
  * text_length
  *  returns the length of the contained text
  */
int text_length(struct Text *text);
 /*
  * text_width
  *  returns the width of the contained text
  */
int text_width(struct Text *text);
 /*
  * text_left
  *  returns the width of the contained text
  */
int text_left(struct Text *text);
 /*
  * text_right
  *  returns the right of the contained text
  */
int text_right(struct Text *text);

 /*
  * text_rect
  *  returns the rect of the contained text
  */
struct Rect text_rect(struct Text *text);
 /*
  * text_bounds
  *  like text_rect, but returns a rect that includes the MAX(height, font size)
  */
struct Rect text_bounds(struct Text *text);

 /*
  * text_baseline
  *  returns the baseline of the contained text (0 is top of page, increases going down)
  */
int text_baseline(struct Text *text);

 /*
  * text_height
  *  returns the height of the contained text (0 is top of page, increases going down)
  */
int text_height(struct Text *text);

 /*
  * text_leading
  *  returns the height of the contained text (0 is top of page, increases going down)
  */
int text_leading(struct Text *text,  struct Text *other);
 /*
  * text_top
  *  returns the height of the contained text (0 is top of page, increases going down)
  */
int text_top(struct Text *text);
 /*
  * text_font
  *  returns the font of the contained text
  */
struct Atom *text_font(struct Text *text);
struct Atom *text_fontprintstr(struct Text *text);
 /*
  * text_fontsize
  *  returns the font of the contained text
  */
int text_fontsize(struct Text *text);

 /*
  * text_intersects
  *  returns 1 if the bounding rects intersect vertically, 0 otherwise
  */
int text_intersects(struct Text *text, struct Text *other);

 /*
  * text_compare
  *  returns -1 if this is before other in page
  *  return 1 if this is after other in page
  *  return 0 if the same position
  *  order is first top to bottom, and then for texts on the same line,
  *  left to right.
  */
int text_compare(struct Text *text, struct Text *other);

 /*
  * text_print
  *  print the contents of text
  */
void text_print(struct Text *text);

 /*
  * text_endsinpunc
  *  returns 1 if text ends in punctuation (.!?) character
  */
int text_endsinpunc(struct Text *text);

 /*
  * text_startswithquote
  *  returns 1 if text start with a capital letter or a digit
  */
int text_startswithquote(struct Text *text);

 /*
  * text_startswithcap
  *  returns 1 if text start with a capital letter or a digit
  */
int text_startswithcap(struct Text *text);

int text_printtext(struct Text *text, struct Text *next, int isparastart, FILE *outfile);

struct array;

 /*
  * text_printcontents
  *  prints the contents of text objects into files
  */
void text_printcontents(struct array *texts, FILE *file, int start, int last);

#endif /* INCLUDED_TEXT_H */
