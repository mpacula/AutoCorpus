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

class Textifier 
{
private:
  State state;
  std::stack<State> stateStack;
  std::string errorContext;

  std::string get_snippet(size_t pos);
  
  bool starts_with(std::string& str);
  bool starts_with(const char* str);
  const char* get_remaining();
  char* get_current_out();
  void skip_match();
  void skip_line();
  void append_group_and_skip(int group);
  void newline(int count);

  void do_link();
  void do_format();
  void do_tag();
  void do_meta_box();
  void do_meta_pipe();
  void do_heading();
  void do_comment();
  void do_list();
  void ignore_nested(std::string name, char open, char close);

  bool get_link_boundaries(int& start, int& end, int& next);

  bool at_line_start(const char* str, int pos);

  std::string get_err(std::string name);
  std::string* match(std::string name, pcre* regexp);

  pcre* make_pcre(const char* expr, int options);
  pcre* re_format;
  pcre* re_heading;
  pcre* re_comment;

public:
  bool ignoreHeadings;

  Textifier();
  ~Textifier();
  int textify(const char* markup, const int markup_len,
              char* out, const int out_len);

  std::string get_error_context();
};

#endif // Textifier_h
