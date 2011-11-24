/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.
    
    utilities.h: see utilities.cpp for details.


    
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

#include <sstream>
#include <pcre.h>
#include <string.h>

#define NGRAM_SEPARATOR '\t'
#define ABBREVIATION_REGEX "^((\\w\\.)|([A-Z][a-z]\\.))+(\\s*\\w\\.?)?(\\s|$)+"

struct Error
{
  std::string message;
  size_t pos;

  Error(std::string message, size_t pos)
  {
    this->message = message;
    this->pos = pos;
  }

  Error offset(long delta_pos)
  {
    return Error(message, pos+delta_pos);
  }
};

pcre* makePCRE(const char* expr, int options);
long findchr(const char* str, char ch);
char* deconstructNGram(const char* str, char* ngram, long* count);

#endif // utilities_h
