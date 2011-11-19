/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    SentenceExtractor.cpp: reads input text and splits it into separate
                           sentences, one per line. Paragraphs are
                           separated by exactly one blank line.

                           

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
#include <iostream>

#include "SentenceExtractor.h"
#include "utilities.h"

using namespace std;

SentenceExtractor::SentenceExtractor(ExtractorOptions opts) 
{
  this->opts = opts;
  this->abbreviationMatcher = new PCREMatcher("^[^\\s]\\.(\\s*[^\\s]\\.)+\\s+", 0);
}

SentenceExtractor::~SentenceExtractor() 
{
  delete abbreviationMatcher;
}

char SentenceExtractor::lastWrittenChar() 
{
  return output[output.size()-1];
}

bool SentenceExtractor::isLastWrittenChar(const char* chars) 
{
  if(output.empty())
    return false;
  const int n = strlen(chars);
  const char last = lastWrittenChar();
  for(int i = 0; i < n; i++) {
    if(last == chars[i])
      return true;
  }
  return false;
}

bool SentenceExtractor::isWS(char ch) 
{
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

char SentenceExtractor::peek() {
  return input[pos+1];
}

char SentenceExtractor::peek(size_t n) {
  size_t i = 0;
  while(input[pos + i] != '\0' && ++i < n);
  return i == n ? input[pos+i] : '\0';
}

size_t SentenceExtractor::find(char ch, size_t len) {
  size_t i = pos;
  while(i < pos+len && input[i] != ch && input[i] != '\0') {
    i++;
  }
  return i;
}  

void SentenceExtractor::newline(int count) 
{
  if(output.size() == 0)
    return;

  for(int i = output.size()-1; i >= 0 && output[i] == '\n'; i--, count--);  

  while(count-- > 0) {
    output += '\n';
  }
}

bool SentenceExtractor::outEndsWith(const char* suffix)
{
  return output.find(suffix, output.size()-strlen(suffix)) != string::npos;
}

string SentenceExtractor::extract(const char* input) 
{
  this->input = input;
  this->pos = 0;
  this->output.clear();
  // Reserve 2MB for output. Most wikipedia articles will fit without resizing.
  this->output.reserve(2*1024*1024); 
  this->len = strlen(input);

  while(input[pos] != '\0') {
    if(abbreviationMatcher->match(&input[pos], len-pos)) {
      string& abbrv = (*abbreviationMatcher)[0];
       output += abbrv;
      pos += abbrv.length();
      if(isupper(input[pos]))
        newline(1);
      continue;
    }

    const char ch = input[pos];
    switch(ch) {
    case '\n':
      if(peek() == '\n' && opts.separateParagraphs) {
        newline(2);
        pos++;
      }
      else if(!isLastWrittenChar(" \t\n") && output.length() > 0) {
        output += ' ';
      }
      break;
      
    case '.':
      output += ch;
      if((isWS(peek()) || peek() == '"' || peek() == '\'')) {
        const size_t nextPeriod = find('.', 5);
        if(false) { // a "sentence" of < 3 letters. Probably an abbreviation.
          while(pos <= nextPeriod) {
            output += input[++pos];
          }
          continue;
        }
        else {
          if(peek()=='"' || peek()=='\'')    
            output += input[++pos];
          
          newline(1);
        }
      }
      break;

    case '?':
    case '!':
      output += ch;
      newline(1);
      break;      

    case ' ':
    case '\t':
      if(output.length() > 0 && !isLastWrittenChar(" \t\r\n"))
        output += ch;
      break;

    default:
      output += ch;
      break;
    }
    
    pos++;
  }

  // make sure we have two newlines at the end: this is so that we
  // can concatenate outputs from multiple articles and have their
  // paragraphs clearly separated.
  newline(2);

  return this->output;
}
