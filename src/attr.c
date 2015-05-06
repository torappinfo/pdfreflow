/*
 * attr.c 
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
#include "hashtable.h"
 /* atoms contains the atoms that are the names of the attributes */
static struct hashtable *atoms = 0;
 /*
  * attrs contains values of the attributes of the current parsed tag 
  * Each key is an atom (a uniqued string)
  */
static struct hashtable *attrs = 0;

 /*
  * attr_init
  *  init all the statics
  */
static void attr_init() {
    atoms = hashtable_string_create(47);
    attrs = hashtable_create(47);
    hashtable_set_keys_are_pointers(attrs);
}

 /*
  * attr_reset
  *  clear out the current attribute name/value pairs
  */
void attr_reset() {
    if (!atoms)
	attr_init();
    hashtable_clean(attrs);
}

 /*
  * attr_atom
  *  create an atom or unique string
  */
struct Atom *attr_atom(char *key) {
    struct Atom *val;
    if (!atoms)
	attr_init();
    val = (struct Atom *) hashtable_get(atoms, key);
    if (!val) {
	hashtable_put(atoms, key, key);
	val = (struct Atom *) hashtable_get(atoms, key);
    }
    return val;
}

 /*
  * attr_put
  *  store an attribute name/value pair
  */
void attr_put(char *key, char *value) {
    struct Atom *atom = attr_atom(key);
    hashtable_put(attrs, (void *)atom, value);
}

 /*
  * attr_get
  *  get a value for attribute atom (must be called with atoms)
  */
char *attr_get(struct Atom *atom) {
    if (!atoms)
	attr_init();
    return (char *) hashtable_get(attrs, (void *)atom);
}
