/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    SentenceExtractor.h: see SentenceExtractor.cpp for a description.


    
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
#ifndef SentenceExtractor_h
#define SentenceExtractor_h

#include <string>

typedef struct _ExtractorOptions
{
  bool separateParagraphs; // separate paragraphs with newlines in output?
} ExtractorOptions;

class SentenceExtractor
{
 private:
  const char* input;
  size_t pos;
  std::string output;
  ExtractorOptions opts;

  bool outEndsWith(const char*);

  char lastWrittenChar();
  char peek();
  char peek(size_t n);
  bool isLastWrittenChar(const char*);
  bool isWS(char);
  void newline(int);

 public:
  SentenceExtractor(ExtractorOptions);
  ~SentenceExtractor(); 
  
  std::string extract(const char*);  
};

#endif // SentenceExtractor_h
