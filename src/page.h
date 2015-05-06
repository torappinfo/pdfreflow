/*
 * page.h 
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


#ifndef INCLUDED_PAGE_H
#define INCLUDED_PAGE_H
struct Page;
struct Text;
struct hashtable;
struct array;
#include "rect.h"
#include <stdio.h>


 /*
  * page_init
  *  create a page object
  */
struct Page *page_init(int number, int width, int height);

 /*
  * page_free
  *  frees a page object
  */
void page_free(struct Page *page);

 /*
  * page_addtext
  *  add a Text to a page
  */
void page_addtext(struct Page *page, struct Text *text);

 /*
  * page_length
  *  return the number of Texts in a page
  */
int page_length(struct Page *page);

 /*
  * page_textat
  *  return Text at index
  */
struct Text *page_textat(struct Page *page, int index);

 /*
  * page_printpageno
  *  print to stderr where in the text the page number is
  */
void page_printpageno(struct Page *page);

 /*
  * page_printchapter
  *  print to stderr if page might be a chapter header
  */
void page_printchapter(struct Page *page, int fontsize);
 /*
  * page_printpara
  *  print to stderr paragraph grouping
  */
void page_printpara(struct Page *page, struct Rect minbounds, struct array *texts, struct hashtable *pagestarts);

 /*
  * page_initpara
  *  inits some stuff in a page object -- is this necessary now ???
  */
void page_initpara(struct Page *page, struct Rect minbounds);

 /*
  * page_multipleparas
  *  enumerates all text objects and generates html, styles, and some debugging info
  */
void page_multipleparas(FILE *file, struct array *texts, struct hashtable *pagestarts, struct Rect minbounds);

 /*
  * page_print
  *  print to stderr paragraph grouping
  */
void page_print(struct Page *page);

 /*
  * page_textbounds
  *  returns the bounding rect of all the text objects
  */
struct Rect page_textbounds(struct Page *page);

 /*
  * page_getleadings
  *  place leading values into the hashtable. key=leading, val=count
  */
void page_getleadings(struct Page *page, struct hashtable *hash, int bias);

 /*
  * page_getleft
  *  place left positions of text objects into the hashtable. key=left, val=count
  */
void page_getleft(struct Page *page, struct hashtable *hash);

 /*
  * page_getright
  *  place right positions of text objects into the hashtable. key=right, val=count
  */
void page_getright(struct Page *page, struct hashtable *hash);

 /*
  * page_getfontsize
  *  place font sizes of text objects into the hashtable. key=fontsize, val=count
  */
void page_getfontsize(struct Page *page, struct hashtable *hash);

 /*
  * page_setlineleading
  *  sets the most common line leading in document
  */
void page_setlineleading(int leading);

 /*
  * page_getlineheight
  *  place line height of text objects into the hashtable. key=height, val=count
  */
void page_getlineheight(struct Page *page, struct hashtable *hash);

 /*
  * page_setlineheight
  *  sets the most common lineheight in document
  */
void page_setlineheight(int lineheight);

 /*
  * page_setindent
  *  sets the x pos an indented line
  */
void page_setindent(int indent);

 /*
  * page_cropbottom
  *  crop text objects whose top is greater than "maxtop"
  */
void page_cropbottom(int maxtop);

 /*
  * page_croptop
  *  crop text objects whose top is less than "mintop"
  */
void page_croptop(int mintop);

 /*
  * page_sort
  *  sort text objects in page
  */
void page_sort(struct Page *page);

 /*
  * page_setrag
  *  set no-justify, or rag-right
  */
void page_setrag();

 /*
  * put_int
  *  if leading exists in hashtable, increment value. 
  *  if force, then alway increment
  *  returns 1 if successful, 0 if nothing happened
  */
int put_int(struct hashtable *hash, int leading, int force);

 /*
  * setPageRange
  *  parse page ranges for --dontreflow option, ie "1-3,4,10, 99-102"
  *  very strict about input, no accomodation for whitespace
  */
void setPageRange(char *range);


 /*
  * page_setshowpara
  *  debugging option --showpara
  */
void page_setshowpara();

 /*
  * page_setcenter
  *  the line (zero base) on this page is a centered line
  */
void page_setcenter(struct Page *page, int lineno);

 /*
  * page_printcontents
  *  print the text lines of a page
  */
void page_printcontents(struct Page *page, FILE *file);

 /*
  * page_setnonfiction
  *  text does not have a lot of dialog, not dependent on indent
  */
void page_setnonfiction();

 /*
  * page_setshortlines
  *  break paragraphs on short lines
  */
void page_setshortlines(int percent);

#endif /* INCLUDED_PAGE_H */ 
