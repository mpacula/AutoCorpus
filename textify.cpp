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

typedef struct _State
{
  int N; // input length
  int pos; // current position in input
  const char* markup; // the markup input we're converting
  char* out;
  int M; // maximum length of output
  int pos_out;
  string groups[10]; // will store regexp matches
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
  void append_group_and_skip(int group);

  void do_link();
  void do_format();
  void do_tag();
  void do_meta();
  void do_nowiki();
  void do_heading();
  void do_nested(string name, char open, char close);

  string get_snippet();
  string get_err(string name);
  string* match(string name, pcre* regexp);

  pcre* make_pcre(const char* expr, int options);
  pcre* re_link;
  pcre* re_nowiki;
  pcre* re_format;
  pcre* re_heading;

public:
  Textifier();
  ~Textifier();
  char* textify(const char* markup, const int markup_len,
                char* out, const int out_len);

  void find_location(long& line, long& col);
};


Textifier::Textifier()
{  
  // Compile all the regexes we'll need
  re_link    = make_pcre("^\\[+((\\w|\\s)*?\\|)*([^\\]]*?)\\]+", 0);
  re_nowiki  = make_pcre("^<nowiki>(.*?)</nowiki>", PCRE_MULTILINE | PCRE_DOTALL);
  re_format  = make_pcre("^('+)(.*?)\\1", 0);
  re_heading = make_pcre("^(=+)\\s*(.+?)\\s*\\1", 0);
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

void Textifier::find_location(long& line, long& column)
{
  line = 1;
  column = 1;
  for(int i = 0; i <= state.pos && i < state.N; i++) {
    if(state.markup[i] == '\n')
      line++;
    else if(line == 1)
      column++;
  }
}

bool Textifier::starts_with(string& str)
{
  return starts_with(str.c_str());
}

bool Textifier::starts_with(const char* str)
{
  int i = state.pos;
  int j = 0;
  while(str[j] != '\0' &&
        i < state.N) {
    if(state.markup[i] != str[j])
      return false;

    i++;
    j++;
  }
  return true;
}

string Textifier::get_snippet()
{
  char snippet[30];
  strncpy(snippet, get_remaining(), 30);
  snippet[min(29, state.N-state.pos)] = '\0';
  return string(snippet);
}

string Textifier::get_err(string name)
{
  ostringstream os;
  os << "Expected " << name << " at '" << get_snippet() << "'";
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
    //cout << i << ": " << substr << endl;
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

void Textifier::append_group_and_skip(int group)
{
  string* val = &state.groups[group];
  strncpy(get_current_out(), val->c_str(), val->length());
  state.pos += state.groups[0].length();
  state.pos_out += val->length();
}

void Textifier::do_link() 
{
  if(!match(string("link"), re_link))
  {
    // Apparently mediawiki allows unmatched open brackets...
    // If that's what we got, it's not a link.
    state.out[state.pos_out++] = state.markup[state.pos++];      
    return;
  }
  if(state.groups[3].find(':') != string::npos)
    skip_match(); // skip links to same page in other languages
  else
    append_group_and_skip(3);
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
  do_nested(string("tag"), '<', '>');
}

void Textifier::do_meta()
{
  do_nested(string("meta"), '{', '}');
}

void Textifier::do_nowiki()
{
  if(!match(string("nowiki"), re_nowiki))
    throw get_err("nowiki");
  
  skip_match();
}

void Textifier::do_format()
{
  if(!match(string("format"), re_format))
    throw get_err("format");
  
  string* contents = &state.groups[2];
  State state_copy = state; // Save state. A call to textify() will reset it.

  // this call will write textified result directly to state.out
  textify(contents->c_str(), contents->length(), 
          &state.out[state.pos_out], state.M-state.pos_out);

  // output has possibly advanced since we saved the state
  state_copy.pos_out += state.pos_out;

  // restore state
  state = state_copy;
  skip_match();
}

void Textifier::do_nested(string name, char open, char close) 
{
  if(state.markup[state.pos] != open)
    throw get_err(name);
  
  int count = 0;
  do {
    if(state.markup[state.pos] == open)       count++;
    else if(state.markup[state.pos] == close) count--;
  } while(state.pos++ < state.N && count > 0);
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
    if(starts_with("<nowiki>"))
       do_nowiki();
    else if(starts_with("["))
      do_link();
    else if(starts_with("<"))
      do_tag();
    else if(starts_with("{{") || starts_with("{|"))
      do_meta();
    else if(starts_with("="))
      do_heading();
    else if(starts_with("''"))
      do_format();
    else {
      out[state.pos_out++] = state.markup[state.pos++];
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
    cerr << "ERROR (" << path << ":" << line << ":" << column << ")  " << err << endl;
    return 1;
  } 
  delete plaintext;
  delete markup;
  return 0;
}
