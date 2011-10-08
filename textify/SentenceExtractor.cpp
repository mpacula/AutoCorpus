#include <string.h>
#include <string>

#include "SentenceExtractor.h"

using namespace std;

SentenceExtractor::SentenceExtractor(ExtractorOptions opts) {
  this->opts = opts;
}

SentenceExtractor::~SentenceExtractor() {
}

char SentenceExtractor::last_written_char() {
  return output[output.size()-1];
}

bool SentenceExtractor::is_last_written_char(const char* chars) {
  if(output.empty())
    return false;
  const int n = strlen(chars);
  const char last = last_written_char();
  for(int i = 0; i < n; i++) {
    if(last == chars[i])
      return true;
  }
  return false;
}

bool SentenceExtractor::is_ws(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

char SentenceExtractor::peek() {
  return input[pos+1];
}

string SentenceExtractor::extract(const char* input) {
  this->input = input;
  this->pos = 0;
  this->output.clear();
  // Reserve 2MB for output. Most wikipedia articles will fit without resizing.
  this->output.reserve(2*1024*1024); 

  while(input[pos] != '\0') {
    const char ch = input[pos];
    switch(ch) {
    case '\n':
      if(opts.separateParagraphs) {
        if(is_last_written_char("\n"))
          output += "\n";
        else
          output += "\n\n";
      }
      break;
      
    case '.':
      output += ch;
      if(is_ws(peek())) {
        if(!is_last_written_char("\n"))
          output += "\n";
      }
      break;

    case '?':
    case '!':
    case '*':
      if(!is_last_written_char("\n"))
        output += "\n";
      break;      

    case ' ':
    case '\t':
      if(pos == 0 || !is_last_written_char(" \t\r\n"))
        output += ch;
      break;

    default:
      output += ch;
      break;
    }
    
    pos++;
  }
  
  return this->output;
}
