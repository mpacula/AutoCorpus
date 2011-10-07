/*
 * Converts media wiki markup into plaintext
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <pcre.h>
#include <math.h>
#include <dirent.h>

using namespace std;

char* substr(char* dest, const char* src, int start, int len, int n) 
{
  if(start >= n)
    throw string("Start index outside of string.");

  int actual_length = min(len, n-start);
  strncpy(dest, &src[start], actual_length);
  dest[actual_length] = '\0';
  return dest;
}

typedef struct _State
{
  int         N;          // input length
  int         pos;        // current position within input
  const char* markup;     // the markup input we're converting
  char*       out;        // output string
  int         M;          // maximum length of output without the terminating \0
  int         pos_out;    // position within output string
  string      groups[10]; // will store regexp matches
} State;

class Textifier 
{
private:
  State state;

  bool starts_with(string& str);
  bool starts_with(const char* str);
  const char* get_remaining();
  char* get_current_out();
  void skip_match();
  void skip_line();
  void append_group_and_skip(int group);

  void do_link();
  void do_format();
  void do_tag();
  void do_meta_box();
  void do_meta_pipe();
  void do_heading();
  void do_comment();
  void ignore_nested(string name, char open, char close);

  bool get_link_boundaries(int& start, int& end, int& next);

  string get_err(string name);
  string* match(string name, pcre* regexp);

  pcre* make_pcre(const char* expr, int options);
  pcre* re_nowiki;
  pcre* re_format;
  pcre* re_heading;
  pcre* re_comment;

public:
  Textifier();
  ~Textifier();
  char* textify(const char* markup, const int markup_len,
                char* out, const int out_len);

  void find_location(long& line, long& col);
  string get_snippet();
};


Textifier::Textifier()
{  
  // Compile all the regexes we'll need
  re_nowiki  = make_pcre("^<nowiki>(.*?)</nowiki>", PCRE_MULTILINE | PCRE_DOTALL);
  re_format  = make_pcre("^(''+)(.*?)(\\1|\n)", 0);
  re_heading = make_pcre("^(=+)\\s*(.+?)\\s*\\1", 0);
  re_comment = make_pcre("<!--.*?-->", PCRE_MULTILINE | PCRE_DOTALL);
}

Textifier::~Textifier()
{
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
    throw string(os.str());
  }
  return re;
}

bool Textifier::get_link_boundaries(int& start, int& end, int& next)
{
  int i = state.pos;   // current search position
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
        start = i+1;
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

void Textifier::find_location(long& line, long& column)
{
  line = 1;
  column = 0;
  for(int i = 0; i <= state.pos && i < state.N; i++) {
    if(state.markup[i] == '\n') {
      line++;
      column = 0;
    }
    else
      column++;
  }
}

bool Textifier::starts_with(string& str)
{
  return starts_with(str.c_str());
}

bool Textifier::starts_with(const char* str)
{
  if(state.N-state.pos < strlen(str))
    return false;

  int i = state.pos;
  int j = 0;
  while(str[j] != '\0' &&
        i < state.N) {
    if(state.markup[i] != str[j])
      return false;

    i++;
    j++;
  }
  return j == strlen(str);
}

string Textifier::get_snippet()
{
  char snippet[30];
  strncpy(snippet, get_remaining(), 30);
  
  const int snippet_len = min(29, state.N-state.pos);
  snippet[snippet_len] = '\0';

  if(snippet_len < state.N - state.pos)
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
    throw get_err(name);

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
  int start, end, next;
  if(get_link_boundaries(start, end, next)) {
    char contents[end-start+1];
    substr(contents, state.markup, start, end-start, state.N);
    if(strchr(contents, ':') != NULL) {
      // this is a link to the page in a different language ignore it
      state.pos = next;
    }
    else {    
      State state_copy = state;
      try {
        textify(contents, end-start, &state.out[state.pos_out], state.M-state.pos_out);
      } 
      catch(string error) {
        state_copy.pos = start + state.pos; // move the pointer to where recursive call failed
        state = state_copy;
        throw error;
      }
      state_copy.pos_out += state.pos_out;
      state_copy.pos = next;
      state = state_copy;
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
  append_group_and_skip(2);
}

void Textifier::do_tag()
{
  int level = 0;
  bool closed = false;

  do {
    char ch = state.markup[state.pos];
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
}

void Textifier::do_comment()
{
  if(!match(string("comment"), re_comment))  
    throw get_err("comment");

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

void Textifier::ignore_nested(string name, char open, char close) 
{
  if(state.markup[state.pos] != open)
    throw get_err(name);
  
  int level = 0;
  do {
    if(state.markup[state.pos] == open)       level++;
    else if(state.markup[state.pos] == close) level--;
  } while(state.pos++ < state.N && level > 0);
}

/**
 * Converts state.markup to plain text. */
char* Textifier::textify(const char* markup, const int markup_len,
                         char* out, const int out_len)
{
  this->state.N = markup_len;
  this->state.pos = 0;
  this->state.markup = markup;
  this->state.out = out;
  this->state.M = out_len;
  this->state.pos_out = 0;

  while(state.pos < state.N && state.pos_out < state.M) {
    if(starts_with("["))
      do_link();
    else if(starts_with("<!--"))
      do_comment();
    else if(starts_with("<"))
      do_tag();
    else if(starts_with("{{") || starts_with("{|"))
      do_meta_box();
    else if(starts_with("|") && (state.pos==0 || state.markup[state.pos-1]=='\n'))
      do_meta_pipe();
    else if(starts_with("="))
      do_heading();
    else if(starts_with("''"))
      do_format();
    else {
      if(state.pos_out == 0 ||
         state.out[state.pos_out-1] != state.markup[state.pos] ||
         state.markup[state.pos] != '\n')
        out[state.pos_out++] = state.markup[state.pos++];
      else
        state.pos++;
    }
  }

  out[state.pos_out] = '\0';

  return out;
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    cerr << "Usage: " << argv[0] << " input-file" << endl;
    return 1;
  }

  char* path = argv[1];
  ifstream file(path, ios::in|ios::ate);
  if(!file) {
    cerr << "The file '" << path << "' does not exist" << endl;
    return 1;
  }
  long size = file.tellg();
  char* markup = new char[size+1];

  file.seekg(0, ios::beg);
  file.read(markup, size);
  markup[size] = '\0';
  file.close();
  
  Textifier tf;
  const int markup_len = strlen(markup);
  char* plaintext = new char[markup_len+1];
  try {
    cout << tf.textify(markup, markup_len, plaintext, markup_len) << endl;
  }
  catch(string err) {
    long line;
    long column;
    tf.find_location(line, column);
    cerr << "ERROR (" << path << ":" << line << ":" << column << ")  " << err
	 << " at: " << tf.get_snippet() << endl;
    return 1;
  } 
  delete plaintext;
  delete markup;
  return 0;
}
