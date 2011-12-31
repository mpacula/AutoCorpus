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
#include <fstream>
#include <string>
#include <vector>
#include <pcre.h>
#include <string.h>
#include "time.h"

#define NGRAM_SEPARATOR '\t'
#define COUNT_SEPARATOR NGRAM_SEPARATOR
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
char* deconstructCount(const char* str, char* ngram, long* count);
void eta(timespec start, unsigned int current, unsigned int total,
         unsigned int* hours, unsigned int* minutes, unsigned int* seconds);
double readProgress(std::ifstream& file, long size);
void words(std::string& str, std::vector<std::string>& vec);
void words(char* str, std::vector<std::string>& vec);

#endif // utilities_h
