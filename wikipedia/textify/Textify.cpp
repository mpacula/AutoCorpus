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

#include "Textifier.h"

using namespace std;

void find_location(const char* input, const size_t pos, long& line, long& column)
{
  line = 1;
  column = 0;
  for(size_t i = 0; i <= pos && input[i] != '\0'; i++) {
    if(input[i] == '\n') {
      line++;
      column = 0;
    }
    else
      column++;
  }
}

void print_usage(char** argv) 
{
  cerr << "Usage: " << argv[0] << " <stdin>" << endl;
}

int main(int argc, char** argv) 
{
  Textifier tf;
  for(int i=1; i<argc; i++) {
    if(strcmp(argv[i], "--ignore-headings") == 0 ||
       strcmp(argv[i], "-h") == 0)
      tf.ignoreHeadings = true;
    else {
      print_usage(argv);
      return 1;
    }
  }

  string input;
  string line;
  long article_start_index = 0, line_number = 0;
  while(getline(cin, line)) {
    line_number++;
    if(line == "\f") { // end of article
      const int markup_len = input.size();
      char* plaintext = new char[2*markup_len+1];
      try {
        tf.textify(input.c_str(), markup_len, plaintext, 2*markup_len);
        cout << plaintext << endl;
        cout << '\f' << endl;
      }
      catch(Error err) {
        long line;
        long column;
        find_location(input.c_str(), err.pos, line, column);
        cerr << "ERROR (" << line+article_start_index << ":" << column << ")  " << err.message
             << " at: " << tf.get_error_context() << endl;
      } 
      delete[] plaintext;
      input.clear();
      article_start_index = line_number+1;
    }
    else {
      input += line;
      input += "\n";
    }
  }

  return 0;
}
