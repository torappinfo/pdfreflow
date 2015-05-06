/*
 * rect.h 
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

#ifndef INCLUDED_RECT_H
#define INCLUDED_RECT_H

struct Rect {
    int top;
    int left;
    int width;
    int height;
};

struct Bounds {
    int minx;
    int miny;
    int maxx;
    int maxy;
};


 /*
  * rect_bottom
  *  calcs bottom of rect
  */
int rect_bottom(struct Rect a);

 /*
  * rect_right
  *  calcs right of rect
  */
int rect_right(struct Rect a);

 /*
  * rect_add
  *  union of two rects
  */
struct Rect rect_add(struct Rect a, struct Rect b);

 /*
  * rect_intersects
  *  returns 1 if rects intersect
  */
int rect_intersects(struct Rect a, struct Rect b);

 /*
  * rect_intersect
  *  returns a rect that is the intersection of two rects
  */
struct Rect rect_intersect(struct Rect a, struct Rect b);

 /*
  * rect_to_bounds
  *  converts rect to min/max x,y
  */
struct Bounds rect_to_bounds(struct Rect r);

 /*
  * rect_from_bounds
  *  converts min/max xy to rect
  */
struct Rect rect_from_bounds(struct Bounds b);

 /*
  * rect_empty
  *  an empty rect
  */
extern struct Rect rect_empty;

 /*
  * rect_min
  *  returns min(a,b)
  */
int rect_min(int a, int b);

 /*
  * rect_max
  *  returns max(a,b)
  */
int rect_max(int a, int b);

#endif /* INCLUDED_RECT_H */
