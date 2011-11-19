/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    PCREMatcher.h: see PCREMatcher.cpp for details.



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
#ifndef PCREMatcher_h
#define PCREMatcher_h

#include <pcre.h>
#include <string>

class PCREMatcher
{
 private:
    std::string groups[10];
    pcre* regexp;
 public:
    PCREMatcher(std::string pcre, const int flags);
    ~PCREMatcher();
    std::string& operator[] (const int index);
    bool match(const std::string& str);
    bool match(const char* str, const int len);

};

#endif
