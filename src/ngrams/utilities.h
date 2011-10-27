#ifndef utilities_h
#define utilities_h

#include <string.h>

#define SEPARATOR '\t'

char* deconstructNGram(const char* str, char* ngram, long* count) {
  const char* delim = strchr(str, SEPARATOR);
  if(delim == NULL)
    return NULL;

  const size_t delimIndex = (delim - str)/sizeof(char);

  strncpy(ngram, &delim[1], strlen(str)-delimIndex);
  sscanf(str, "%ld", count);
  return ngram;
}

#endif
