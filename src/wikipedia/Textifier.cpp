#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <pcre.h>
#include <math.h>

#include "Textifier.h"

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

bool is_prefix(const char* str, const char* sub, const size_t n)
{
  size_t i = 0;
  while(sub[i] != '\0' &&
        i < n) {
    if(str[i] != sub[i])
      return false;
    i++;
  }
  return i == strlen(sub);
}

bool is_substr(const char* str, const char* substr, int n)
{
  const int m = strlen(substr);
  while(m <= n) {
    if(is_prefix(str, substr, n))
      return true;

    str = &str[1];
    n--;
  }
  
  return false;
}

Textifier::Textifier()
{  
  ignoreHeadings = false;

  // Compile all the regexes we'll need
  re_format  = make_pcre("^(''+)(.*?)(\\1|\n)", 0);
  re_heading = make_pcre("^(=+)\\s*(.+?)\\s*\\1", 0);
  re_comment = make_pcre("<!--.*?-->", PCRE_MULTILINE | PCRE_DOTALL);

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

pcre* Textifier::make_pcre(const char* expr, int options)
{
  const char* error;
  int erroffset;
  pcre *re = pcre_compile(expr, options, &error, &erroffset, NULL);

  if(re == NULL) {
    ostringstream os;
    os << "PCRE compilation failed at offset " << erroffset << ": "
       << error << endl;
    throw Error(os.str(), state.pos);
  }
  return re;
}

bool Textifier::get_link_boundaries(int& start, int& end, int& next)
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

string Textifier::get_error_context() {
  return errorContext;
}

bool Textifier::starts_with(string& str)
{
  return starts_with(str.c_str());
}

bool Textifier::starts_with(const char* str)
{
  return is_prefix(&state.markup[state.pos], str, state.N-state.pos);
}

string Textifier::get_snippet(size_t pos)
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

string Textifier::get_err(string name)
{
  ostringstream os;
  os << "Expected markup type '" << name << "'";
  return os.str();
}

string* Textifier::match(string name, pcre* regexp) 
{
  const int ovector_size = 3*sizeof(state.groups)/sizeof(string);
  int ovector[ovector_size];
  int rc = pcre_exec(regexp, NULL, get_remaining(), state.N-state.pos, 0, 0, ovector, ovector_size);
 
  if(rc == PCRE_ERROR_NOMATCH || rc == 0)
    return NULL;
  else if(rc < 0)
    throw Error(get_err(name), state.pos);

  // from pcredemo.c
  for(int i = 0; i < rc; i++) {
    const char *substring_start = get_remaining() + ovector[2*i];
    int substring_length = ovector[2*i+1] - ovector[2*i];
    char substr[substring_length+1];
    strncpy(substr, substring_start, substring_length);
    substr[substring_length]='\0';
    state.groups[i].assign(substr);
  }
  return state.groups;
}

const char* Textifier::get_remaining()
{
  return &state.markup[state.pos];
}

char* Textifier::get_current_out()
{
  return &state.out[state.pos_out];
}

void Textifier::skip_match()
{
  state.pos += state.groups[0].length();
}

void Textifier::skip_line()
{
  while(state.pos < state.N && state.markup[state.pos++] != '\n');
}

void Textifier::append_group_and_skip(int group)
{
  string* val = &state.groups[group];
  strncpy(get_current_out(), val->c_str(), val->length());
  state.pos += state.groups[0].length();
  state.pos_out += val->length();
}

void Textifier::do_link() 
{
  int start = 0, end = 0, next = 0;
  if(get_link_boundaries(start, end, next)) {
    char contents[end-start+1];
    substr(contents, state.markup, start, end-start, state.N);

    // is this a file link? if so, put it in its own paragraph
    const bool fileLink = is_substr(&state.markup[state.pos], "File:", start-state.pos);
    if(fileLink) {
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

    if(fileLink) {
      newline(2);
    }
  } else {
    // Apparently mediawiki allows unmatched open brackets...
    // If that's what we got, it's not a link.
    state.out[state.pos_out++] = state.markup[state.pos++];      
    return;
  }
}

void Textifier::do_heading() 
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
    skip_match();
  else {
    append_group_and_skip(2);
    newline(2);    
  }
}

void Textifier::do_tag()
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

void Textifier::do_comment()
{
  if(!match(string("comment"), re_comment))  
    throw Error(get_err("comment"), state.pos);

  skip_match();
}

void Textifier::do_meta_box()
{
  ignore_nested(string("meta"), '{', '}');
}

void Textifier::do_meta_pipe()
{
  skip_line();
}

void Textifier::do_format()
{
  // ignore all immediate occurences of two or more apostrophes
  while(state.pos < state.N && state.markup[state.pos] == '\'') {
    state.pos++;
  }
}

void Textifier::do_list()
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
        !is_prefix(&state.markup[end_index], "<!--", state.N-end_index)) {
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

void Textifier::ignore_nested(string name, char open, char close) 
{
  if(state.markup[state.pos] != open)
    throw Error(get_err(name), state.pos);
  
  int level = 0;
  do {
    if(state.markup[state.pos] == open)       level++;
    else if(state.markup[state.pos] == close) level--;
  } while(state.pos++ < state.N && level > 0);
}

void Textifier::newline(int count) 
{
  if(state.pos_out == 0)
    return;

  for(int i = state.pos_out-1; i >= 0 && state.out[i] == '\n'; i--, count--);  

  while(count-- > 0) {
    state.out[state.pos_out++] = '\n';
  }
}

bool Textifier::at_line_start(const char* str, int pos) {
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
        if(starts_with("["))
          do_link();
        else if(starts_with("<!--"))
          do_comment();
        else if(starts_with("<"))
          do_tag();
        else if(starts_with("{{") || starts_with("{|"))
          do_meta_box();
        else if(starts_with("|") && at_line_start(state.markup, state.pos))
          do_meta_pipe();
        else if(at_line_start(state.out, state.pos_out) && (starts_with("*") || starts_with("-")))
          do_list();
        else if(at_line_start(state.out, state.pos_out) && starts_with(":"))
          state.pos++;
        else if(starts_with("="))
          do_heading();
        else if(starts_with("''"))
          do_format();
        else {
          state.out[state.pos_out++] = state.markup[state.pos++];
        }
      }
    }
    catch(Error err) {
      errorContext = get_snippet(err.pos);
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
