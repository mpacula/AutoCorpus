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
