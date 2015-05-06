/*
 * html.h 
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


#ifndef INCLUDED_HTML_H
#define INCLUDED_HTML_H

#include "rect.h"
#include <stdio.h>

 /*
  * enum para_style
  *  align_unknown - initial state, alignment is unknown
  *  align_left - left aligned paragraph
  *  align_center - center aligned paragraph
  *  align_right - right aligned paragraph
  *  align_indent - left aligned paragraph with left indent (index > left margin)
  *  align_outdent - left aligned paragraph with left outdent (index <  left margin)
  */
enum para_style {
    align_unknown, align_left, align_center, align_right, align_indent, align_outdent,
    align_blockquote
};
struct array;
struct Atom;
struct Line;

 /*
  * html_printtexts
  *  prints the paragraph text
  */
void html_printtexts(FILE *file, struct array *texts, struct Line *startline, int endindex, enum para_style style,  struct Rect r, int otherval, int left, int width);

 /*
  * html_notestyle
  *  records a style, which is uniqued
  */
void html_notestyle(struct array *texts, int startindex, int endindex, enum para_style style,  struct Rect r, int otherval, int left, int width);

 /*
  * html_printstyles
  *  enumerate styles for css tag in head section
  */
void html_printstyles(FILE *file);

 /*
  * html_setrag
  *  tells that the text is rag right
  */
void html_setrag();

 /*
  * html_setshowstyle
  *  print debugging info when styles are created
  */
void html_setshowstyle();

 /*
  * html_printtail
  *  prints the end of the html file
  */
void html_printtail(FILE *file);

 /*
  * parstylestr
  *  get print string for para_style
  */
char *html_parstylestr(enum para_style style);

 /*
  * html_setdefaultfont
  *  sets the default font for document -- printstr format Atom, ie Times-12
  */
void html_setdefaultfont(struct Atom *defaultfont);

 /*
  * html_setabsolute
  *  use absolute font sizes in output, rather than relative (percentage)
  */
void html_setabsolute();

#endif /* INCLUDED_HTML_H */ 
