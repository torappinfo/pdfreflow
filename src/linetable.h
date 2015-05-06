/*
 * linetable.h 
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

#ifndef INCLUDED_LINETABLE_H
#define INCLUDED_LINETABLE_H

#include "rect.h"
struct array;
struct Text;
struct Line {
    struct Text *text;
    int index;
    struct Rect r;
};

 /*
  * enumlines
  *  enumerates each unique line of the texts, that is, if multple texts are on the
  *  same line, only the first text of the line is enumerated, the rest of the line is
  *  skipped.
  */
void linetable_enumlines(struct array *texts, void *context, int (*proc)(void *context, struct Text *cur, struct Text *next, struct Rect rect, int index, int nextindex));

 /*
  * linetable_enum_linetable
  *  enumerate the lines in the XML document.
  *  if save is 1, then remember the line table for the next iteration. 
  *  if save is 1, and there is a previous line table, use it.
  *  if save is 0, then empty line table.
  *  proc gets a context pointer, the current and next Line structs.
  *  if proc returns 0, end enumeration
  */
void linetable_enum_linetable(struct array *texts, void *context, int save, int (*proc)(void *context, struct Line *cur, struct Line *next));

#endif /* INCLUDED_LINETABLE_H */
