/*
 * parse.h 
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


#ifndef INCLUDED_PARSE_H
#define INCLUDED_PARSE_H

#include <stdio.h>
struct array;
 /*
  * parse_pdf2xml
  *  parses a pdf2xml text file
  *  returns a array of Pages
  */
struct array *parse_pdf2xml(FILE *file);
 /*
  * parse_encoding
  *  returns the character encoding
  */
char *parse_encoding();

#endif /* INCLUDED_PARSE_H */
