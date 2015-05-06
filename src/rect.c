/*
 * rect.c 
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

struct Rect rect_empty = {0};

 /*
  * rect_min
  *  returns min(a,b)
  */
int rect_min(int a, int b) {
    if (a < b)
	return a;
    return b;
}

 /*
  * rect_max
  *  returns max(a,b)
  */
int rect_max(int a, int b) {
    if (a > b)
	return a;
    return b;
}

 /*
  * rect_bottom
  *  calcs bottom of rect
  */
int rect_bottom(struct Rect a) {
    return a.top + a.height;
}

 /*
  * rect_right
  *  calcs right of rect
  */
int rect_right(struct Rect a) {
    return a.left + a.width;
}


 /*
  * rect_add
  *  union of two rects
  */
struct Rect rect_add(struct Rect a, struct Rect b) {
    struct Rect val;
    int bot = rect_max(rect_bottom(a), rect_bottom(b));
    int right = rect_max(rect_right(a), rect_right(b));
    val.top = rect_min(a.top, b.top);
    val.left = rect_min(a.left, b.left);
    val.height = bot - val.top;
    val.width = right - val.left;
    return val;
}


 /*
  * rect_to_bounds
  *  converts rect to min/max x,y
  */
struct Bounds rect_to_bounds(struct Rect r) {
    struct Bounds b;
    b.minx = r.left;
    b.miny = r.top;
    b.maxx = rect_right(r);
    b.maxy = rect_bottom(r);
    return b;
}


 /*
  * rect_from_bounds
  *  converts min/max xy to rect
  */
struct Rect rect_from_bounds(struct Bounds b) {
    struct Rect r;
    r.top = b.miny;
    r.left = b.minx;
    r.width = b.maxx - b.minx;
    r.height = b.maxy - b.miny;
    return r;
}


 /*
  * rect_intersects
  *  returns 1 if rects intersect
  */
int rect_intersects(struct Rect a, struct Rect b) {
    struct Bounds b_a = rect_to_bounds(a);
    struct Bounds b_b = rect_to_bounds(b);
    if (b_b.maxx <= b_a.minx)
	return 0;
    if (b_a.maxx <= b_b.minx)
	return 0;
    if (b_b.maxy <= b_a.miny)
	return 0;
    if (b_a.maxy <= b_b.miny)
	return 0;
    return 1;
}


 /*
  * rect_intersect
  *  returns a rect that is the intersection of two rects
  */
struct Rect rect_intersect(struct Rect a, struct Rect b) {
    struct Bounds b_a, b_b;
    struct Rect r;
    if (!rect_intersects(a, b))
	return rect_empty;
    b_a = rect_to_bounds(a);
    b_b = rect_to_bounds(b);
    r.left = rect_max(b_a.minx, b_b.minx);
    r.top = rect_max(b_a.miny, b_b.miny);
    r.width = rect_min(b_a.maxx, b_b.maxx);
    r.height = rect_min(b_a.maxy, b_b.maxy);
    return r;
}