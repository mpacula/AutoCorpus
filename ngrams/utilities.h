#ifndef utilities_h
#define utilities_h

#include <string.h>

#define SEPARATOR '\t'

char* deconstructNGram(const char* str, char* ngram, long* count) {
  const char* delim = strchr(str, SEPARATOR);
  if(delim == NULL)
    return NULL;

  strncpy(ngram, str, (size_t)(delim-str));
  ngram[(size_t)(delim-str)] = '\0';
  sscanf(&delim[1], "%ld", count);
  return ngram;
}

#endif
