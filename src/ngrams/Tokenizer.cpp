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

#define PUNCTUATION ".,!?()&@()[]{}/\\\"'#:;<>^”*=-−—\x93"

using namespace std;

class Tokenizer
{
private:
  const char* keep; // punctuation to include in output  
  bool includeParens;
  char lastChar;
  
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

  inline void space()
  {
    if(!isWS(lastChar)) {
      fputc(' ', stdout);
      lastChar = ' ';
    }
  }

  inline void character(char ch)
  {
    fputc(ch, stdout);
    lastChar = ch;
  }
  
public:
  Tokenizer(const char* keep, bool includeParens) {
    this->keep = keep;
    this->includeParens = includeParens;
    this->lastChar = '\0';
  }

  void tokenize(string input) {
    int parenLevel = 0;
    for(size_t i=0; i < input.length(); i++) {
      char ch = input[i];
      switch(ch) {
      case '(':
        parenLevel++;
        if(!includeParens) // skip over the '('
          continue;
        else {
          space();
          character('(');
          space();
          continue;
        }
        break;
      case ')':
        parenLevel--;
        if(!includeParens) // skip over the ')'
          continue;
        else {
          space();
          character(')');
          space();
          continue;
        }
        break;
      }
      
      if(parenLevel == 0 || includeParens) {
        if(isWS(ch))
          space();
        else if(isPunctuation(ch) && isKeep(ch)) {
          space();
          character(ch);
          space();
        }
        else if(isPunctuation(ch)) {
          // don't break words on commas delimiting orders of magnitude
          // in numbers, e.g. 1,000,000
          if(ch == ',' && isDigit(lastChar) && isDigit(input[i+1]))
            character(ch);
          else
            space();
        }
        else if(!isPunctuation(ch))
          character(tolower(ch));
      }
    }

    character('\n');
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
