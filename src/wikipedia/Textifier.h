/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    Textifier.h: see Textifier.cpp for a description.

    

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
#ifndef Textifier_h
#define Textifier_h

#include <pcre.h>
#include <string>
#include <stack>

typedef struct _state
{
  size_t           N;          // input length
  size_t           pos;        // current position within input
  const char*      markup;     // the markup input we're converting
  char*            out;        // output std::string
  size_t           M;          // maximum length of output without the terminating \0
  size_t           pos_out;    // position within output std::string
  std::string      groups[10]; // stores regexp matches
} State;

class Textifier 
{
private:
  State state;
  std::stack<State> stateStack;
  std::string errorContext;

  std::string getSnippet(size_t pos);
  
  bool startsWith(std::string& str);
  bool startsWith(const char* str);
  const char* getRemaining();
  char* getCurrentOut();
  void skipMatch();
  void skipLine();
  void appendGroupAndSkip(int group);
  void newline(int count);

  void doLink();
  void doFormat();
  void doTag();
  void doMetaBox();
  void doMetaPipe();
  void doHeading();
  void doComment();
  void doList();
  void ignoreNested(std::string name, char open, char close);

  bool getLinkBoundaries(int& start, int& end, int& next);

  bool atLineStart(const char* str, int pos);

  std::string getErrorMessage(std::string name);
  std::string* match(std::string name, pcre* regexp);

  pcre* re_format;
  pcre* re_heading;
  pcre* re_comment;

public:
  bool ignoreHeadings;

  Textifier();
  ~Textifier();
  int textify(const char* markup, const int markup_len,
              char* out, const int out_len);

  std::string getErrorContext();
};

#endif // Textifier_h
