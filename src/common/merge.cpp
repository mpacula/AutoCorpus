/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    merge.cpp: utilities for merging files with counts


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

#include <stdlib.h>
#include <ctype.h>
#include <iostream>

#include "merge.h"
#include "utilities.h"

using namespace std;

const size_t buf_size = 1024*1024;

char* read_line(char* buf, size_t n, FILE* f, bool& hasMore)
{
  char* result;
  hasMore = (result = fgets(buf, n, f)) != NULL;
  return result;
}  

void readCount(FILE* f, char* key, long& count, bool& hasMore)
{
  char line[buf_size];
  do {
    read_line(line, buf_size, f, hasMore);  
    if(hasMore && !deconstructCount(line, key, &count)) {
      cerr << "WARNING: Could not deconstruct count from line:" << endl;
      cerr << line;
      continue;
    } else {
      break;
    }
  } while(hasMore); // loops until we get a successful parse of EOF
}

void inline printCount(FILE* out, const char* key, const long count)
{
  fprintf(out, "%ld%c%s\n", count, COUNT_SEPARATOR, key);
}

/// Merges two sorted files with counts into a third sorted file with
/// the sum of counts. Returns the sum of counts.
int mergeCounts(FILE* src1, FILE* src2, FILE* out)
{ 
  bool hasMore1, hasMore2;
  char key1[buf_size];
  char key2[buf_size];
  long c1=0, c2=0, c_total=0;

  readCount(src1, key1, c1, hasMore1);
  readCount(src2, key2, c2, hasMore2);

  while(hasMore1 || hasMore2) {
    /* Both counts present? */
    if(hasMore1 && hasMore2) {
      int cmp = strcmp(key1, key2);
      if(cmp == 0) {
        /* words from both chunks equal: add counts */
        printCount(out, key1, c1+c2);
        c_total+=(c1+c2);
        readCount(src1, key1, c1, hasMore1);
        readCount(src2, key2, c2, hasMore2);
        continue;
      }
      /* word from first chunk is smaller? */
      else if(cmp < 0) {
        printCount(out, key1, c1);
        c_total += c1;
        readCount(src1, key1, c1, hasMore1);
        continue;
      }
      /* word from second chunk is smaller? */
      else {
        printCount(out, key2, c2);
        c_total += c2;
        readCount(src2, key2, c2, hasMore2);
        continue;
      }
    }
    /* Only first line nonempty */
    else if(hasMore1) {
      printCount(out, key1, c1);
      c_total += c1;
      readCount(src1, key1, c1, hasMore1);
    } 
    /* Only second line nonempty */
    else if(hasMore2) {
      printCount(out, key2, c2);
      c_total += c2;
      readCount(src2, key2, c2, hasMore2);
    }
  }

  if(!feof(src1) || !feof(src2))
    throw string("Read error. All counts have been processed, but end of file has not been reached.");
  
  return c_total;
}
