/** Splits input into tokens (words) */

#include <string.h>
#include <iostream>
#include <string>

#define PUNCTUATION ".,!?()&@()[]{}/\\\""

using namespace std;

class Tokenizer
{
private:
  const char* keep; // punctuation to include in output  
  bool includeParens;
  
  inline bool isPunctuation(char ch) 
  {
    return strchr(PUNCTUATION, ch);
  }

  inline bool isKeep(char ch) 
  {
    return strchr(keep, ch);
  }

  inline bool isWS(char ch)
  {
    return strchr(" \t\r\n\f", ch);
  }
  
public:
  Tokenizer(const char* keep, bool includeParens) {
    this->keep = keep;
    this->includeParens = includeParens;
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
          fputs("( ", stdout);
          continue;
        }
        break;
      case ')':
        parenLevel--;
        if(!includeParens) // skip over the ')'
          continue;
        else {
          fputs(") ", stdout);
          continue;
        }
        break;
      }
      
      if(parenLevel == 0 || includeParens) {
        if(isWS(ch))
          fputs(" ", stdout);
        else if(isPunctuation(ch) && isKeep(ch))
          printf(" %c ", ch);
        else if(!isPunctuation(ch))
          fputc(ch, stdout);
      }
    }

    puts("");
  } // end of tokenize()
};

void printUsage(const char* name)
{
  printf("Usage: %s [--keep CHARACTERS] [--parens]\n", name);
}

int main(int argc, const char** argv)
{
  bool includeParens = false;
  const char* keep;

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
