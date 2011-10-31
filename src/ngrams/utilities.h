/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    Copyright (C) 2011 Maciej Pacula


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef utilities_h
#define utilities_h

#include <string.h>
#include <algorithm>
#include <iostream>

#define SEPARATOR '\t'

long findchr(const char* str, char ch)
{
  const char* offset = strchr(str, ch);
  if(offset == NULL)
    return -1;
  return (long)(offset - str);
}

char* deconstructNGram(const char* str, char* ngram, long* count) 
{
  const long delimIndex = findchr(str, SEPARATOR);
  if(delimIndex < 0)
    return NULL;

  // length until newline or end of string
  long len = findchr(str, '\r'); 
  if(len < 0)
    len = findchr(str, '\n');
  if(len < 0)
    len = findchr(str, '\0');
  if(len < 0)
    return NULL;

  strncpy(ngram, &str[delimIndex+1], len-delimIndex-1);
  ngram[len-delimIndex-1] = '\0';
  sscanf(str, "%ld", count);
  return ngram;
}

#endif
