#include <string.h>
#include <string>

#include "SentenceExtractor.h"

using namespace std;

SentenceExtractor::SentenceExtractor(ExtractorOptions opts) 
{
  this->opts = opts;
}

SentenceExtractor::~SentenceExtractor() 
{
}

char SentenceExtractor::last_written_char() 
{
  return output[output.size()-1];
}

bool SentenceExtractor::is_last_written_char(const char* chars) 
{
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

bool SentenceExtractor::is_ws(char ch) 
{
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

char SentenceExtractor::peek() {
  return input[pos+1];
}

void SentenceExtractor::newline(int count) 
{
  for(int i = output.size()-1; i >= 0 && output[i] == '\n'; i--, count--);  

  while(count-- > 0) {
    output += '\n';
  }
}

string SentenceExtractor::extract(const char* input) 
{
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
        newline(2);
      }
      break;
      
    case '.':
      output += ch;
      if(is_ws(peek())) {
        newline(1);
      }
      break;

    case '?':
    case '!':
      newline(1);
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
