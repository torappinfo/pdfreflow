/*
 * hashtable.h 
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


#ifndef INCLUDED_HASHTABLE_H
#define INCLUDED_HASHTABLE_H
struct hashtable;

#define HASHTABLE_ERROR ((void *)-1)

struct hashtable *hashtable_create(int size);
void *hashtable_put(struct hashtable *table, const char *key, void *value);
void *hashtable_get(struct hashtable *table, const char *key);
void *hashtable_remove(struct hashtable *table, const char *key);
int hashtable_length(struct hashtable *table);
void hashtable_free(struct hashtable *table);

void hashtable_enumerate(struct hashtable *table, int (*proc)(const char *key, void *value, void *context), void *context);
	/* proc returns 0 to continue, anything else to stop enumeration */

struct hashtable_procs {
    void (*mp_free)(struct hashtable *table, void *value, void *context);
};

 /*
  * creates a hashtable that contains strings as its values.  Values
  * are copied as they are put into the table, and free upon
  * removal
  */
struct hashtable *hashtable_string_create(int size);

 /*
  * special creation procs, where values need special attention with regard
  * to freeing
  */
struct hashtable *hashtable_special_create(int size, struct hashtable_procs *procs, void *context);

/*
 * deletes all entries in the hashtable. If a free function 
 * has been specified in hashtable_procs, then it will be called as each
 * entry is deleted.
 */
void hashtable_clean(struct hashtable *this);

void hashtable_free_values(struct hashtable *table);
	/*
	 * tells hashtable to call free() on value when values are overidden
	 * with hashtable_put() or on hashtable_destroy()
	 */
void hashtable_copy_keys(struct hashtable *table, int copy);
	/* if copy is nonzero, then keys are copied when values are
	 * put into the hashtable.  By default, copying keys is
	 * enabled
	 */

void hashtable_set_keys_are_pointers(struct hashtable *table);
struct array;
struct array *hashtable_keys(struct hashtable *table, int copystrings);

#endif /* INCLUDED_HASHTABLE_H */
