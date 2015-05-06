/*
 * hashtable.c 
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
#include "hashtable.h"
#include "array.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct hashelement {
    const char *he_key;
    void *he_value;
    int he_hash;
};

struct hashtable {
    int ht_count;
    int ht_allocated;
    int ht_freevalues;
    int ht_dont_malloc_keys;
    int ht_keys_are_pointers;
    int ht_contains_strings;
    void *ht_context;
    struct hashelement *ht_table;
    struct hashtable_procs *ht_procs;
};

static int calcSize(int oldSize) {
    int start = 8;
    int size = start - 1;
    int minSize = 2 * oldSize;
    while (size < minSize) {
	start *= 2;
    	size = start - 1;
    }
    return size;
}

static int hash(struct hashtable *table, const char *key, int *sum) {
    int ch, value = 0;
    const char *s = key;
    int length, shift = 0;
    if (table->ht_keys_are_pointers)
	return (int)key % table->ht_allocated;
    length = strlen(key) % 128;
    while ((ch = *s)) {
	value += (ch << ((ch & 0xf) + (shift % 4))) - ch;
	s++;
	shift++;
    }
    value = ((value * value) >> 8) | (length << 24) ;
    value = value & 0x7fffffff;
    if (sum)
	*sum = value;
    return value % table->ht_allocated;
}

struct hashelement *
alloctable(struct hashtable *table, int size) {
    table->ht_table = (struct hashelement *)malloc(size * sizeof(struct hashelement));
    if (table->ht_table) {
	memset(table->ht_table, 0, (size * sizeof(struct hashelement)));
	table->ht_allocated = size;
    }
    return table->ht_table;
}

struct hashtable *hashtable_special_create(int size, struct hashtable_procs *procs, void *context) {
    struct hashtable *table;
    if (!size)
	size = 5;
    size = calcSize(size);
    table = (struct hashtable *)malloc(sizeof(struct hashtable));
    if (table) {
	memset(table, 0, sizeof(struct hashtable));
	table->ht_procs = procs;
	table->ht_context = context;
	if (!alloctable(table, size)) {
	    free(table);
	    table = NULL;
	}
    }
    return table;
}
struct hashtable *hashtable_create(int size) {
    return hashtable_special_create(size, NULL, NULL);
}

struct hashtable *hashtable_string_create(int size) {
    struct hashtable *table = hashtable_special_create(size, NULL, NULL);
    if (table) {
	table->ht_contains_strings = 1;
	hashtable_free_values(table);
    }
    return table;
}

static int
mystrcmp(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}

static struct hashelement *
getentry(struct hashtable *table, const char *key, int *hashp) {
    int sum = 0;
    int start = hash(table, key, &sum);
    int index = start;
    struct hashelement *cur;
    if (hashp)
	*hashp = sum;
    do {
	cur = table->ht_table + index;
	if (!cur->he_key)
	    return cur;
	else if (cur->he_key == key)
	    return cur;
	else if (!table->ht_keys_are_pointers && 
	    sum == cur->he_hash && !mystrcmp(cur->he_key, key))
	    return cur;
	index = (index + 1) % table->ht_allocated;
    } while (index != start);
    return NULL;
}

static void *cleanup(struct hashtable *table, void *old) {
    if (table->ht_procs && table->ht_procs->mp_free) {
	table->ht_procs->mp_free(table, old, table->ht_context);
	old = NULL;
    } else if (table->ht_freevalues) {
	free(old);
	old = NULL;
    }
    return old;
}

static void *put(struct hashtable *table, const char *key, void *value, int copykey, int *grew) {
    int sum = 0;
    struct hashelement *entry = getentry(table, key, &sum);
    void *old = entry->he_value;
    
    if (grew)
        *grew = (old == NULL);
    entry->he_value = value;
    entry->he_hash = sum;
    if (!entry->he_key) {
	if (copykey && !table->ht_dont_malloc_keys) {
	    entry->he_key = strdup(key);
	    if (!entry->he_key)
		return HASHTABLE_ERROR;
	} else
	    entry->he_key = key;
	entry->he_hash = sum;
    } else {
	if (table->ht_dont_malloc_keys)
	    entry->he_key = key;
    }
    if (old)
	old = cleanup(table, old);
    return old;
}

static struct hashelement *rehash(struct hashtable *table) {
    struct hashelement *old = table->ht_table;
    int oldSize = table->ht_allocated;
    int newSize = calcSize(oldSize);
    struct hashelement *cur, *last = old + oldSize;
    if (alloctable(table, newSize)) {
	for (cur = old; cur < last; cur++) {
	    if (cur->he_key) {
		if (put(table, cur->he_key, cur->he_value, 0, NULL) == HASHTABLE_ERROR)
		    return NULL;
	    }
	}
    }
    free(old);
    return table->ht_table;
}

static int getcount(struct hashtable *table, struct hashelement *entry) {
    int i, index = entry - table->ht_table;
    int count = 0;
    for (i = 0; i < table->ht_allocated; i++) {
	struct hashelement *cur = table->ht_table + ((index + i) % table->ht_allocated);
	if (!cur->he_key)
	    break;
	count++;
    }
    return count;
}
static void getcopy(struct hashtable *table, struct hashelement *copy, struct hashelement *entry, int count) {
    int i, index = entry - table->ht_table;
    for (i = 0; i < count; i++) {
	struct hashelement *cur = table->ht_table + ((index + i) % table->ht_allocated);
	copy->he_key = cur->he_key;
	copy->he_value = cur->he_value;
	cur->he_key = NULL;
	cur->he_value = NULL;
	cur->he_hash = 0;
	copy++;
    }
}

void *hashtable_remove(struct hashtable *table, const char *key) {
    struct hashelement *entry;
    void *result = NULL;
    if (!table->ht_table)
	return HASHTABLE_ERROR;
    entry = getentry(table, key, NULL);
    if (entry && entry->he_key) {
	int i, count = getcount(table, entry);
	char *oldkey;
	result = entry->he_value;
	oldkey = (char *)entry->he_key;
	if (count > 1) {
	    struct hashelement *copy = (struct hashelement *)malloc(count * sizeof(struct hashelement));
	    if (!copy)
		return HASHTABLE_ERROR;
	    getcopy(table, copy, entry, count);
	    for (i = 1; i < count; i++) {
		struct hashelement *cur = copy + i;
		if (put(table, cur->he_key, cur->he_value, 0, NULL) == HASHTABLE_ERROR)
		    return HASHTABLE_ERROR;
	    }
	    free(copy);
	} else {
	    entry->he_key = NULL;
	    entry->he_value = NULL;
	    entry->he_hash = 0;
	}
	result = cleanup(table, result);
	if (!table->ht_dont_malloc_keys)
	    free(oldkey);
	table->ht_count--;
    }
    return result;
}

void *hashtable_get(struct hashtable *table, const char *key) {
    struct hashelement *entry;
    if (!table->ht_table)
	return HASHTABLE_ERROR;
    entry = getentry(table, key, NULL);
    return entry ? entry->he_value : NULL;
}

int hashtable_length(struct hashtable *table) {
    return table->ht_count;
}


void *hashtable_put(struct hashtable *table, const char *key, void *value) {
    void *old;
    int maxusage = (3 * table->ht_allocated) / 4;
    int grew;
    
    if (!table->ht_table)
	return HASHTABLE_ERROR;
    if ((table->ht_count + 1) >= maxusage)
	if (!rehash(table))
	    return HASHTABLE_ERROR;
    if (table->ht_contains_strings) {
	value = strdup(value);
	if (!value)
	    return HASHTABLE_ERROR;
    }
    old = put(table, key, value, 1, &grew);
    if (grew)
	table->ht_count++;
    return old;
}

void hashtable_enumerate(struct hashtable *table, int (*proc)(const char *key, void *value, void *context), void *context) {
    struct hashelement *start, *last;
    if (!table->ht_table)
	return;
    start = table->ht_table;
    last = start + table->ht_allocated;
    for (; start < last; start++) {
	if (start->he_key) {
	    if ((*proc)(start->he_key, start->he_value, context))
		return;
	}
    }
}

void hashtable_free_values(struct hashtable *table) {
    table->ht_freevalues = 1;
}

void hashtable_copy_keys(struct hashtable *table, int copy) {
    table->ht_dont_malloc_keys = copy ? 0 : 1;
}

void hashtable_set_keys_are_pointers(struct hashtable *table) {
    table->ht_keys_are_pointers = 1;
    table->ht_dont_malloc_keys = 1;
}

void hashtable_free(struct hashtable *table) {
    if (table->ht_table) {
	struct hashelement *start = table->ht_table;
	struct hashelement *last = start + table->ht_allocated;
	for (; start < last; start++) {
	    if (start->he_key) {
		start->he_value = cleanup(table, start->he_value);
		if (!table->ht_dont_malloc_keys)
		    free((char *)start->he_key);
		start->he_key = NULL;
		start->he_hash = 0;
	    }
	}
	free(table->ht_table);
    }
    free(table);
}

struct array *hashtable_keys(struct hashtable *table, int copystrings) {
    struct array *result = copystrings ? array_string_init(0) : array_pointer_init(0);
    if (table->ht_table && result) {
	struct hashelement *start = table->ht_table;
	struct hashelement *last = start + table->ht_allocated;
	for (; start < last; start++) {
	    if (start->he_key) {
		array_append_element(result, (char *)start->he_key);
	    }
	}
    }
    return result;
}

void hashtable_clean(struct hashtable *table) {
    struct hashelement *start, *last;
    if (!table->ht_table)
	return;
    start = table->ht_table;
    last = start + table->ht_allocated;
    for (; start < last; start++) {
	if (start->he_key) {
	    start->he_value = cleanup(table, start->he_value);
	    if (!table->ht_dont_malloc_keys)
		free((char *)start->he_key);
	    start->he_key = NULL;
	    start->he_hash = 0;
	    start->he_value = NULL;
	}
    }
    table->ht_count = 0;
}
