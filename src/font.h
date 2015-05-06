/*
 * font.h 
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


#ifndef INCLUDED_FONT_H
#define INCLUDED_FONT_H
struct Atom;

 /*
  * font_add
  *  Add a font to the global table.
  */
void font_add(char *id, int size, char *family, char *color);
 /*
  * font_size
  *  returns the size of a font, the id of the font is an atom
  */
int font_size(struct Atom *id);
 /*
  * font_family
  *  returns the family of a font, the id of the font is an atom
  */
char *font_family(struct Atom *id);

 /*
  * font_printfonts
  *  print all the fonts in the document
  */
void font_printfonts();

struct Atom *font_printstr(struct Atom *id);
struct Atom *font_fromprintstr(struct Atom *printstr);

#endif /* INCLUDED_FONT_H */
