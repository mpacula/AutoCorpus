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

char SentenceExtractor::peek(size_t n) {
  size_t i = 0;
  while(input[pos + i] != '\0' && ++i < n);
  return i == n ? input[pos+i] : '\0';
}

void SentenceExtractor::newline(int count) 
{
  if(output.size() == 0)
    return;

  for(int i = output.size()-1; i >= 0 && output[i] == '\n'; i--, count--);  

  while(count-- > 0) {
    output += '\n';
  }
}

bool SentenceExtractor::out_ends_with(const char* suffix)
{
  return output.find(suffix, output.size()-strlen(suffix)) != string::npos;
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
      if(peek() == '\n' && opts.separateParagraphs) {
        newline(2);
        pos++;
      }
      else if(!is_last_written_char(" \t")) {
        output += ' ';
      }
      break;
      
    case '.':
      output += ch;
      if((is_ws(peek()) || peek() == '"' || peek() == '\'') &&
         !out_ends_with("e.g.") &&
         !out_ends_with("i.e.")) {
        if(peek(3) == '.') { // One letter "sentence": most likely an abbreviation
          output += input[pos+1];
          output += input[pos+2];
          output += input[pos+3];
          pos += 3;
        }
        else {
          if(peek()=='"' || peek()=='\'') {         
            output += input[++pos];
          }
          
          newline(1);
        }
      }
      break;

    case '?':
    case '!':
      output += ch;
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

  // make sure we have two newlines at the end: this is so that we
  // can concatenate outputs from multiple articles and have their
  // paragraphs clearly separated.
  newline(2);

  return this->output;
}
