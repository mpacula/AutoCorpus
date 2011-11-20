/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    PCREMatcher.cpp: a simple wrapper around a regexp that stores matched
                     groups as STL strings, easily accessible through
                     the subscript operator.



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
#include <string.h>
#include <string>

#include "PCREMatcher.h"
#include "utilities.h"

using namespace std;

PCREMatcher::PCREMatcher(string pcre, const int flags) {
  this->regexp = makePCRE(pcre.c_str(), flags);
}  

PCREMatcher::~PCREMatcher() {
  pcre_free(this->regexp);
}

bool PCREMatcher::match(const char* str, const int len) {
  const int ovector_size = 3*sizeof(this->groups)/sizeof(string);
  int ovector[ovector_size];
  int rc = pcre_exec(regexp, NULL, str, len, 0, 0, ovector, ovector_size);
 
  if(rc == PCRE_ERROR_NOMATCH || rc == 0)
    return false;
  else if(rc < 0) {
    ostringstream os;
    os << "PCRE error. Code: " << rc;
    throw Error(os.str(), 0);
  }
 
  // from pcredemo.c
  for(int i = 0; i < rc; i++) {
    const char *substring_start = &str[ovector[2*i]];
    int substring_length = ovector[2*i+1] - ovector[2*i];
    char substr[substring_length+1];
    strncpy(substr, substring_start, substring_length);
    substr[substring_length]='\0';
    this->groups[i].assign(substr);
  }
  return true;
}

bool PCREMatcher::match(const string& str) {
  return this->match(str.c_str(), str.length());
}

string& PCREMatcher::operator[] (const int index) {
  const int max = sizeof(this->groups)/sizeof(string);
  if(index < 0 || index >= max)
    throw Error("PCRE group index out of range.", 0);
  else
    return this->groups[index];
}
