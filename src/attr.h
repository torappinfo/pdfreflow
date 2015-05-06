/*
 * attr.h 
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


#ifndef INCLUDED_ATTR_H
#define INCLUDED_ATTR_H

struct Atom;

 /*
  * attr_reset
  *  clear out the current attribute name/value pairs
  */
void attr_reset();
 /*
  * attr_put
  *  store an attribute name/value pair
  */
void attr_put(char *key, char *value);
 /*
  * attr_get
  *  get a value for attribute atom (must be called with atoms)
  */
char *attr_get(struct Atom *atom);
 /*
  * attr_atom
  *  create an atom or unique string
  */
struct Atom *attr_atom(char *key);

#endif /* INCLUDED_ATTR_H */
