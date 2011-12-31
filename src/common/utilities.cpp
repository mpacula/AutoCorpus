/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    utilities.cpp: various helper functions



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
#include "utilities.h"

using namespace std;

/*
   PCRE
*/
pcre* makePCRE(const char* expr, int options)
{
  const char* error;
  int erroffset;
  pcre *re = pcre_compile(expr, options, &error, &erroffset, NULL);

  if(re == NULL) {
    std::ostringstream os;
    os << "PCRE compilation failed at offset " << erroffset << ": "
            << error << std::endl;
    throw Error(os.str(), 0);
  }
  return re;
}


/*
  NGRAMS
*/

long findchr(const char* str, char ch)
{
  const char* offset = strchr(str, ch);
  if(offset == NULL)
    return -1;
  return (long)(offset - str);
}

char* deconstructCount(const char* str, char* ngram, long* count) 
{
  const long delimIndex = findchr(str, NGRAM_SEPARATOR);
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


/*
  TIME
*/

/// Computes the ETA of a task in hours:minutes:seconds
void eta(timespec start, unsigned int current, unsigned int total,
         unsigned int* hours, unsigned int* minutes, unsigned int* seconds)
{
  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  double elapsed_sec = now.tv_sec - start.tv_sec;
  double speed = (double)current / elapsed_sec;
  double workRemaining = total - current;
  double remaining_sec = workRemaining / speed;
  
  *hours = (int)(remaining_sec / (60*60));
  remaining_sec -= *hours*60*60;
  *minutes = (int)(remaining_sec / 60);
  remaining_sec -= *minutes*60;
  *seconds = remaining_sec;
}

// Gets file read progress, in %
double readProgress(ifstream& file, long fileSize)
{
  return 100.0*file.tellg() / fileSize;
}


/// Splits a sentence into words (tokens separated by one or more
/// spaces).
void words(string& str, vector<string>& vec) 
{
  size_t start = 0;
  for(size_t i = 0; i < str.size(); i++) {
    if(isspace(str[i])) {
      if(i > start) {
        vec.push_back(str.substr(start, i-start));        
      } 
      start = i+1;
    }
  }

  if(start < str.size()) {
    vec.push_back(str.substr(start, str.size()));
  }
}

void words(char* str, vector<string>& vec) 
{
  string buf = str;
  words(buf, vec);
}
