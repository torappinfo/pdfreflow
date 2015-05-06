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


#include <config.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"

struct array {
    int elementSize;
    int length;
    int allocated;
    int contains_pointers;
    void *data;
    void *context;
    struct array_procs *procs;
};

#define ALLOC_SIZE(len) (len + 1)

struct array *
array_init(int elementSize, int length)
{
    struct array *this = (struct array *)malloc(sizeof(struct array));
    if (this) {
	memset(this, 0, sizeof(struct array));
	this->elementSize = elementSize;
	this->length = length;
	this->allocated = ALLOC_SIZE(length);
	this->data = (void *)malloc(this->allocated * elementSize);
	if (this->data) 
	    memset(this->data, 0, this->allocated * elementSize);
	else {
	    array_free(this);	
	    this = NULL;
	}
    }
    return (this);
}

struct array *
array_copy(struct array *old)
{
    struct array *this = (struct array *)malloc(sizeof(struct array));
    if (this) {
	memcpy(this, old, sizeof(struct array));
	this->allocated = ALLOC_SIZE(old->length);
	this->data = (void *)malloc(this->allocated * old->elementSize);
	if (this->data) {
	    memcpy(this->data, old->data, old->length * old->elementSize);
	    if (this->procs && this->procs->ap_append) {
		int i;
		for (i = 0; i < this->length; i++) {
		    this->procs->ap_append(this, this->data + (i * this->elementSize), this->context);
		}
	    }
	} else {
	    array_free(this);	
	    this = NULL;
	}
    }
    return (this);
}

void
array_free(struct array *this)
{
    if (this->procs && this->procs->ap_free && this->data) {
	int i;
	for (i = 0; i < this->length; i++) {
	    this->procs->ap_free(this, this->data + (i * this->elementSize), this->context);
	}
    }
    if (this->data)
	free(this->data);
    free(this);
}

int
array_append_element(struct array *this, void *element)
{
    if (!this->data)
	return -1;
    if (this->length == this->allocated) {
	int newalloc = this->allocated * 2;
        this->data = (void *)realloc((char *)this->data, newalloc * this->elementSize);
	if (!this->data)
	    return -1;
        this->allocated = newalloc;
    }
    if (this->contains_pointers) 
	((void **)this->data)[this->length] = element;
    else
	memcpy(this->data + (this->elementSize * this->length), element, this->elementSize);
    if (this->procs && this->procs->ap_append)
	this->procs->ap_append(this, this->data + (this->elementSize * this->length), this->context);
    this->length++;
    return this->length;
}

void
array_set_elementat(struct array *this, void *element, int index) 
{
    if (!this->data)
	return;
    if (this->procs && this->procs->ap_free)
	this->procs->ap_free(this, this->data + (this->elementSize * index), this->context);
    if (this->contains_pointers) 
	((void **)this->data)[index] = element;
    else
	memcpy(this->data + (this->elementSize * index), element, this->elementSize);
    if (this->procs && this->procs->ap_append)
	this->procs->ap_append(this, this->data + (this->elementSize * index), this->context);
}

int
array_insert_elementat(struct array *this, void *element, int index)
{
    if (!this->data || index < 0 || index >= this->length)
	return -1;
    if (this->length == this->allocated) {
	int newalloc = this->allocated * 2;
        this->data = (void *)realloc((char *)this->data, newalloc * this->elementSize);
	if (!this->data)
	    return -1;
        this->allocated = newalloc;
    }
    memmove(this->data + ((index+1) * this->elementSize), this->data + (index * this->elementSize), ((this->length - index) * this->elementSize));
    if (this->contains_pointers) 
	((void **)this->data)[index] = element;
    else
	memcpy(this->data + (this->elementSize * index), element, this->elementSize);
    if (this->procs && this->procs->ap_append)
	this->procs->ap_append(this, this->data + (this->elementSize * index), this->context);
    this->length++;
    return this->length;
}

void *
array_elementat(struct array *this, int index)
{
    if (!this->data)
	return ARRAY_ERROR;
    if (this->contains_pointers) 
	return ((void **)this->data)[index];
    else
	return this->data + (index * this->elementSize);
}

int
array_element_index(struct array *this, void *element)
{
    int i;
    void **data = (void **)this->data;
    void *structdata = this->data;
    if (!this->data)
	return -1;
    for (i = 0; i < this->length; i++) {
	if (this->contains_pointers) {
	    if (data[i] == element)
		return i;
	    if (this->procs && this->procs->ap_equals) {
		if (this->procs->ap_equals(this, element, data[i], this->context))
		    return i;
	    }
	} else {
	    if (this->procs && this->procs->ap_equals) {
		if (this->procs->ap_equals(this, element, structdata, this->context))
		    return i;
	    } else {
		if (!memcmp(structdata, element, this->elementSize))
		    return i;
	    }
	    structdata += this->elementSize;
	}
    }
    return -1;
}

void
array_remove_element(struct array *this, void *element)
{
    int i;
    if (!this->data)
	return;
    i = array_element_index(this, element);
    if (i >= 0)
	array_remove_elementat(this, i);
}

void
array_remove_elementat(struct array *this, int i)
{
    if (!this->data)
	return;
    if (i < this->length) {
	if (this->procs && this->procs->ap_free)
	    this->procs->ap_free(this, this->data + (i * this->elementSize), this->context);
	memmove(this->data + (i * this->elementSize), this->data + ((i+1) * this->elementSize), ((this->length - i - 1) * this->elementSize));
	this->length--;
    }
}

int
array_length(struct array *this)
{
    if (!this->data)
	return -1;
    return this->length;
}

int
array_setlength(struct array *this, int length)
{
    if (!this->data)
	return -1;
    if (length > this->allocated) {
	int newalloc = ALLOC_SIZE(length);
        this->data = (void *)realloc((char *)this->data, newalloc * this->elementSize);
	if (!this->data)
	    return -1;
        this->allocated = newalloc;
    }
    if (length > this->length) {
	memset(this->data + (this->length * this->elementSize), 0, (this->elementSize * (length - this->length)));
    } else if (length < this->length) {
	if (this->procs && this->procs->ap_free) {
	    int i;
	    for (i = length; i < this->length; i++) {
		this->procs->ap_free(this, this->data + (i * this->elementSize), this->context);
	    }
	}
    }
    this->length = length;
    return length;
}

struct array *
array_special_init(struct array_procs *procs, int length, void *context)
{
    struct array *this;
    if (procs->ap_init)
	this = procs->ap_init(length, context);
    else
	this = array_init(sizeof(void *), length);
    if (this) {
	this->procs = procs;
	this->context = context;
    }
    return this;
}

static void
string_append(struct array *this, void **element, void *context) {
    char **strp = (char ** )element;
    *strp = strdup(*strp);
}

static void
string_free(struct array *this, void **element, void *context) {
    char **strp = (char ** )element;
    free(*strp);
}

static int
string_equals (struct array *this, void *e1, void *e2, void *context) {
    return strcmp(e1, e2) ? 0 : 1;
}

struct array_procs string_procs = {
    NULL, string_append, string_free, string_equals
};

struct array *
array_string_init(int length)
{
    struct array *this = array_special_init(&string_procs, length, NULL);
    if (this)
	array_set_contains_pointers(this, 1);
    return this;
}

static int
compare(const void *p1, const void *p2) {
    char **s1 = (char **)p1;
    char **s2 = (char **)p2;
    return strcmp(*s1, *s2);
}

static int
casecompare(const void *p1, const void *p2) {
    char **s1 = (char **)p1;
    char **s2 = (char **)p2;
    return strcasecmp(*s1, *s2);
}

static int
descendcompare(const void *p1, const void *p2) {
    char **s1 = (char **)p1;
    char **s2 = (char **)p2;
    return strcmp(*s2, *s1);
}

static int
descendcasecompare(const void *p1, const void *p2) {
    char **s1 = (char **)p1;
    char **s2 = (char **)p2;
    return strcasecmp(*s2, *s1);
}

extern void
array_sortstrings(struct array *this, int ascending, int ignoreCase)
{
    if (!this->data)
	return;
    if (ascending) {
	if (ignoreCase)
	    qsort(this->data, this->length, sizeof(char *), casecompare);
	else
	    qsort(this->data, this->length, sizeof(char *), compare);
    } else {
	if (ignoreCase)
	    qsort(this->data, this->length, sizeof(char *), descendcasecompare);
	else
	    qsort(this->data, this->length, sizeof(char *), descendcompare);
    }

}

void
array_set_contains_pointers(struct array *this, int ispointer)
{
    this->contains_pointers = ispointer;
}

struct array *
array_pointer_init(int length) {
    struct array *this = array_init(sizeof(void *), length);
    if (this)
	array_set_contains_pointers(this, 1);
    return this;
}

void *
array_getdata(struct array *this)
{
    return this->data;
}


