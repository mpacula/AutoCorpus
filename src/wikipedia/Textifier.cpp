/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    Textifier.cpp: removes markup and formatting from Wikipedia articles.
                   Preseves paragraphs by separating them with *at least*
                   one blank line.

                   Articles are read line by line from stdin and are
                   assumed to be separated by page feed characters \f
                   on their own line.
                   
                   The line feeds are preserved in the output.

                   
                   
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
#include <sstream>
#include <pcre.h>
#include <math.h>

#include "Textifier.h"
#include "utilities.h"

using namespace std;

char* substr(char* dest, const char* src, int start, int len, int n) 
{
  if(start >= n)
    throw Error("Start index outside of string.", start);

  int actual_length = min(len, n-start);
  strncpy(dest, &src[start], actual_length);
  dest[actual_length] = '\0';
  return dest;
}

bool isPrefix(const char* str, const char* sub, const size_t n, bool ignoreCase)
{
  size_t i = 0;
  while(sub[i] != '\0' &&
        i < n) {
    if((str[i] != sub[i] && !ignoreCase) || (tolower(str[i]) != tolower(sub[i])))
      return false;
    i++;
  }
  return i == strlen(sub);
}

bool isPrefix(const char* str, const char* sub, const size_t n)
{
  return isPrefix(str, sub, n, false);
}

bool isSubstr(const char* str, const char* substr, int n, bool ignoreCase)
{
  const int m = strlen(substr);
  while(m <= n) {
    if(isPrefix(str, substr, n, ignoreCase))
      return true;

    str = &str[1];
    n--;
  }
  
  return false;
}

bool isSubstr(const char* str, const char* substr, int n)
{
  return isSubstr(str, substr, n, false);
}

Textifier::Textifier()
{  
  ignoreHeadings = false;

  // Compile all the regexes we'll need
  re_format  = makePCRE("^(''+)(.*?)(\\1|\n)", 0);
  re_heading = makePCRE("^(=+)\\s*(.+?)\\s*\\1", 0);
  re_comment = makePCRE("<!--.*?-->", PCRE_MULTILINE | PCRE_DOTALL);

  state.markup = NULL;
  state.out = NULL;
  state.pos = state.pos_out = 0;
  state.N = state.M = 0;
}

Textifier::~Textifier()
{
  pcre_free(re_format);
  pcre_free(re_heading);
  pcre_free(re_comment);
}

bool Textifier::getLinkBoundaries(int& start, int& end, int& next)
{
  size_t i = state.pos;   // current search position
  int level = 0; // nesting level
  do {
    char ch = state.markup[i];
    switch(ch) {
    case '[':
      if(level++ == 0)
        start = i+1;
      break;

    case ']':
      if(--level == 0)
        end = i;
      break;

    case '|':
      if(level == 1) { // does the pipe belong to current link or a nested one?
        start = i+1;   // level 1 means current
        end = start;
      }
      break;

    default:
      end++;
      break;
    }

    i++;
  } while(level > 0 &&
          i < state.N);

  next = i;
  return level == 0; // if 0, then brackets match and this is a correct link  
}

string Textifier::getErrorContext() {
  return errorContext;
}

bool Textifier::startsWith(string& str)
{
  return startsWith(str.c_str());
}

bool Textifier::startsWith(const char* str)
{
  return isPrefix(&state.markup[state.pos], str, state.N-state.pos);
}

string Textifier::getSnippet(size_t pos)
{
  if(pos >= state.N)
    return "n/a";

  char snippet[30];
  strncpy(snippet, &state.markup[pos], 30);
  
  const size_t snippet_len = min((size_t)29, state.N-pos);
  snippet[snippet_len] = '\0';

  if(snippet_len < state.N - pos)
    strncpy(&snippet[snippet_len-3], "...", 3); 

  return string(snippet);
}

string Textifier::getErrorMessage(string name)
{
  ostringstream os;
  os << "Expected markup type '" << name << "'";
  return os.str();
}

string* Textifier::match(string name, pcre* regexp) 
{
  const int ovector_size = 3*sizeof(state.groups)/sizeof(string);
  int ovector[ovector_size];
  int rc = pcre_exec(regexp, NULL, getRemaining(), state.N-state.pos, 0, 0, ovector, ovector_size);
 
  if(rc == PCRE_ERROR_NOMATCH || rc == 0)
    return NULL;
  else if(rc < 0)
    throw Error(getErrorMessage(name), state.pos);

  // from pcredemo.c
  for(int i = 0; i < rc; i++) {
    const char *substring_start = getRemaining() + ovector[2*i];
    int substring_length = ovector[2*i+1] - ovector[2*i];
    char substr[substring_length+1];
    strncpy(substr, substring_start, substring_length);
    substr[substring_length]='\0';
    state.groups[i].assign(substr);
  }
  return state.groups;
}

const char* Textifier::getRemaining()
{
  return &state.markup[state.pos];
}

char* Textifier::getCurrentOut()
{
  return &state.out[state.pos_out];
}

void Textifier::skipMatch()
{
  state.pos += state.groups[0].length();
}

void Textifier::skipLine()
{
  while(state.pos < state.N && state.markup[state.pos++] != '\n');
}

void Textifier::appendGroupAndSkip(int group)
{
  string* val = &state.groups[group];
  strncpy(getCurrentOut(), val->c_str(), val->length());
  state.pos += state.groups[0].length();
  state.pos_out += val->length();
}

void Textifier::doLink() 
{
  int start = 0, end = 0, next = 0;
  if(getLinkBoundaries(start, end, next)) {
    char contents[end-start+1];
    substr(contents, state.markup, start, end-start, state.N);

    // is this a file/image link? if so, put it in its own paragraph
    const bool fileLink = isSubstr(&state.markup[state.pos], "File:", start-state.pos, true);
    const bool imageLink = isSubstr(&state.markup[state.pos], "Image:", start-state.pos, true);
    if(fileLink || imageLink) {
      newline(2);
    }

    size_t recursive_bytes_written = 0;
    try {
      recursive_bytes_written = textify(contents, end-start,
                                        &state.out[state.pos_out],
                                        state.M-state.pos_out);
    } 
    catch(Error error) {
      // offset error location by the beginning of the recursive call
      throw error.offset(start);
    }

    if(strchr(&state.out[state.pos_out], ':') == NULL) {
      // This is *not* a link to the page in a different language.
      // (otherwise we ignore it by not moving the out pointer, so its
      // contents get overwritten)
      state.pos_out += recursive_bytes_written;
    }

    state.pos = next;

    if(fileLink || imageLink) {
      newline(2);
    }
  } else {
    // Apparently mediawiki allows unmatched open brackets...
    // If that's what we got, it's not a link.
    state.out[state.pos_out++] = state.markup[state.pos++];      
    return;
  }
}

void Textifier::doHeading() 
{
  if(!match(string("heading"), re_heading))
    {
      // Not really a heading. Just copy to output.
      state.out[state.pos_out++] = state.markup[state.pos++];      
      return;
    }
  else if(state.groups[2] == "References" ||
          state.groups[2] == "Footnotes" ||
          state.groups[2] == "Related pages" ||
          state.groups[2] == "Further reading") {
    state.pos = state.N;
  }
  else if(ignoreHeadings)
    skipMatch();
  else {
    appendGroupAndSkip(2);
    newline(2);    
  }
}

void Textifier::doTag()
{
  int level = 0;
  bool closed = false;
  string tag;
  do {
    char ch = state.markup[state.pos];
    tag += ch;
    switch(ch) {
    case '<':
      level++;
      break;

    case '>':
      level--;
      break;

    case '/':
      closed = (level == 1); // we must be inside the right closing tag
      break;
    }    
    state.pos++;
  } while((level > 0 || !closed) && state.pos < state.N);
  
  if(tag == "<br>" || tag == "<br/>" || tag == "<br />") {
    state.out[state.pos_out++] = '\n';
  }
}

void Textifier::doComment()
{
  if(!match(string("comment"), re_comment))  
    throw Error(getErrorMessage("comment"), state.pos);

  skipMatch();
}

void Textifier::doMetaBox()
{
  ignoreNested(string("meta"), '{', '}');
}

void Textifier::doMetaPipe()
{
  skipLine();
}

void Textifier::doFormat()
{
  // ignore all immediate occurences of two or more apostrophes
  while(state.pos < state.N && state.markup[state.pos] == '\'') {
    state.pos++;
  }
}

void Textifier::doList()
{
  newline(2);
  while(state.pos < state.N &&
        (state.markup[state.pos] == '*' ||
         state.markup[state.pos] == '-' ||
         state.markup[state.pos] == ' ' ||
         state.markup[state.pos] == '\t')) {
    state.pos++;
  }

  int end_index = state.pos;
  while(state.markup[end_index] != '\0' &&
        state.markup[end_index] != '\n' &&
        !isPrefix(&state.markup[end_index], "<!--", state.N-end_index)) {
    end_index++;
  }
  
  size_t item_length = end_index - state.pos;
  size_t bytes_written = textify(&state.markup[state.pos], 
          item_length,
          &state.out[state.pos_out],
          state.M-state.pos_out);
  state.pos += item_length;
  state.pos_out += bytes_written;

  newline(2);
}

void Textifier::ignoreNested(string name, char open, char close) 
{
  if(state.markup[state.pos] != open)
    throw Error(getErrorMessage(name), state.pos);
  
  int level = 0;
  do {
    if(state.markup[state.pos] == open)       level++;
    else if(state.markup[state.pos] == close) level--;
  } while(state.pos++ < state.N && level > 0);
}

void Textifier::newline(int count) 
{
  for(int i = state.pos_out-1; i >= 0 && state.out[i] == '\n'; i--, count--);  

  while(count-- > 0) {
    state.out[state.pos_out++] = '\n';
  }
}

bool Textifier::atLineStart(const char* str, int pos) {
  if(pos == 0)
    return true;

  int i = pos-1;
  while(i >= 0) {
    const char ch = str[i--];
    if(ch == ' ' || ch == '\t' || ch == '\r')
      continue;
    else if(ch == '\n')
      return true;
    else 
      return false;
  }
  return true;
}

/**
 * Converts state.markup to plain text. */
int Textifier::textify(const char* markup, const int markup_len,
                       char* out, const int out_len)
{
  stateStack.push(state);

  this->state.N = markup_len;
  this->state.pos = 0;
  this->state.markup = markup;
  this->state.out = out;
  this->state.M = out_len;
  this->state.pos_out = 0;

  if(state.markup == NULL)
    throw Error("null markup", state.pos);
  
  try {
    while(state.pos < state.N && state.pos_out < state.M) {      
        if(startsWith("["))
          doLink();
        else if(startsWith("<!--"))
          doComment();
        else if(startsWith("<"))
          doTag();
        else if(startsWith("{{") || startsWith("{|"))
          doMetaBox();
        else if(startsWith("|") && atLineStart(state.markup, state.pos))
          doMetaPipe();
        else if(atLineStart(state.out, state.pos_out) && (startsWith("*") || startsWith("-")))
          doList();
        else if(atLineStart(state.out, state.pos_out) && startsWith(":"))
          state.pos++;
        else if(startsWith("="))
          doHeading();
        else if(startsWith("''"))
          doFormat();
        else {
          state.out[state.pos_out++] = state.markup[state.pos++];
        }
      }
    }
    catch(Error err) {
      errorContext = getSnippet(err.pos);
      state = stateStack.top();
      stateStack.pop();
      throw err;
    }

    out[state.pos_out] = '\0';
    const size_t pos_out = state.pos_out;

    state = stateStack.top();
    stateStack.pop();
    return pos_out;
}
