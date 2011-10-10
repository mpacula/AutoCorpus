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
  while(getline(cin, line)) {
    if(line == "\f") { // end of article
      const int markup_len = input.size();
      char* plaintext = new char[2*markup_len+1];
      try {
        cout << tf.textify(input.c_str(), markup_len, plaintext, markup_len) << endl;
        cout << '\f' << endl;
      }
      catch(string err) {
        long line;
        long column;
        tf.find_location(line, column);
        cerr << "ERROR (" << line << ":" << column << ")  " << err
             << " at: " << tf.get_snippet() << endl;
      } 
      delete plaintext;
      input.clear();
    }
    else {
      input += line;
      input += "\n";
    }
  }

  return 0;
}
