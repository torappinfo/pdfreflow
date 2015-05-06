/*
 * array.h 
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


#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

struct array;

#define ARRAY_ERROR ((void *)-1)
struct array *array_init(int elementSize, int length);
void array_free(struct array *this);
struct array *array_copy(struct array *this);

int array_setlength(struct array *this, int length);
int array_length(struct array *this);

int array_append_element(struct array *this, void *element);
void array_remove_element(struct array *this, void *element);
void *array_elementat(struct array *this, int index);
void array_set_elementat(struct array *this, void *element, int index);
void array_remove_elementat(struct array *this, int index);
int array_insert_elementat(struct array *this, void *element, int index);
int array_element_index(struct array *this, void *element);

/* array contains strings, which are copied.  elements appended and
 * returned are string (char *).
 */
struct array *array_string_init(int length); 
extern void array_sortstrings(struct array *this, int ascending, int ignoreCase);

/* array contains pointers.  elementSize is sizeof(void *), and you 
 * can pass in your pointer, rather than the address of your pointer.
 */
struct array *array_pointer_init(int length); 

void array_set_contains_pointers(struct array *this, int ispointer);

struct array_procs {
    struct array *(*ap_init)(int length, void *context);
    void (*ap_append)(struct array *this, void **element, void *context);
    void (*ap_free)(struct array *this, void **element, void *context);
    int (*ap_equals)(struct array *this, void *e1, void *e2, void *context);
};

struct array *array_special_init(struct array_procs *procs, int length, void *context);

void *array_getdata(struct array *this);

#endif /* INCLUDED_ARRAY_H */
