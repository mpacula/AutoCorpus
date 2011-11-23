/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    Tokenizer.cpp: splits input lines into sequences of words separated
                   by exactly one space. Ignores all punctuation by default,
                   but can be easily configured to keep selected characters.

                   Can and by default does ignore all text inside parantheses,
                   in order to make the output easier to use in building NLP
                   language models.

                   

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
#include <ctype.h>
#include <iostream>
#include <string>

#include "PCREMatcher.h"
#include "utilities.h"

#define PUNCTUATION ".,!?()&@()[]{}/\\\"'#:;<>^”*=-−—\x93"

using namespace std;

class Tokenizer
{
private:
  const char* keep; // punctuation to include in output  
  bool includeParens;
  char lastChar;
  PCREMatcher* abbreviationMatcher;  
  
  inline bool isPunctuation(char ch) 
  {
    if(ch == '\'')
      return isWS(lastChar);

    return strchr(PUNCTUATION, ch);
  }

  inline bool isDigit(char ch) 
  {
    return isdigit(ch);
  }

  inline bool isKeep(char ch) 
  {
    return strchr(keep, ch);
  }

  inline bool isWS(char ch)
  {
    return isspace(ch);
  }

  inline void printSpace()
  {
    if(!isWS(lastChar)) {
      fputc(' ', stdout);
      lastChar = ' ';
    }
  }

  inline void printChar(char ch)
  {
    fputc(ch, stdout);
    lastChar = ch;
   }
  
  inline void printString(string& input) {
    for(size_t i = 0; i < input.length(); i++) {
      const char ch = input[i];
      if(ch == ' ')
        printSpace();
      else
        printChar(tolower(ch));
    }
  }

public:
  Tokenizer(const char* keep, bool includeParens) {
    this->keep = keep;
    this->includeParens = includeParens;
    this->lastChar = '\0';
    this->abbreviationMatcher = new PCREMatcher(ABBREVIATION_REGEX, 0);
  }

  ~Tokenizer() {
    delete abbreviationMatcher;
  }

  void tokenize(string& input) {
    int parenLevel = 0;
    for(size_t i=0; i < input.length(); i++) {
      // first check for abbreviations like "U.S."
      if((i == 0 || isWS(input[i-1])) && abbreviationMatcher->match(&(input.c_str()[i]), input.length()-i)) {
        string& abbrv = (*abbreviationMatcher)[0];
        printString(abbrv);
        i += abbrv.length()-1;
        continue;
      }

      char ch = input[i];
      switch(ch) {
      case '(':
        parenLevel++;
        if(!includeParens) // skip over the '('
          continue;
        else {
          printSpace();
          printChar('(');
          printSpace();
          continue;
        }
        break;
      case ')':
        parenLevel--;
        if(!includeParens) // skip over the ')'
          continue;
        else {
          printSpace();
          printChar(')');
          printSpace();
          continue;
        }
        break;
      }
      
      if(parenLevel == 0 || includeParens) {
        if(isWS(ch))
          printSpace();
        else if(isPunctuation(ch) && isKeep(ch)) {
          printSpace();
          printChar(ch);
          printSpace();
        }
        else if(isPunctuation(ch)) {
          // don't break words on commas delimiting orders of magnitude
          // in numbers, e.g. 1,000,000
          if(ch == ',' && isDigit(lastChar) && isDigit(input[i+1]))
            printChar(ch);
          else
            printSpace();
        }
        else if(!isPunctuation(ch))
          printChar(tolower(ch));
      }
    }

    printChar('\n');
  } // end of tokenize()
};

void printUsage(const char* name)
{
  printf("Usage: %s [--keep CHARACTERS] [--parens]\n", name);
}

int main(int argc, const char** argv)
{
  bool includeParens = false;
  const char* keep = "";

  for(int i=1; i<argc; i++) {
    if(strcmp("--keep", argv[i]) == 0 && i<argc-1) {
      keep = argv[i+1];
      i++;
    }
    else if(strcmp("--parens", argv[i]) == 0) {
      includeParens = true;
    }
    else {
      printUsage(argv[0]);
      return 1;
    }
  }

  Tokenizer tokenizer(keep, includeParens);

  string line;
  try {
    while(getline(cin, line)) {
      tokenizer.tokenize(line);
    }
  } catch(string err) {
    cerr << err << endl;
    return 1;
  }

}
