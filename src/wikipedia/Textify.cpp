/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

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
