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

int main(int argc, char** argv) 
{
  if(argc != 1) {
    cerr << "Usage: " << argv[0] << " <stdin>" << endl;
    return 1;
  }

  string input;
  string line;
  while(getline(cin, line)) {
    input += line;
    input += "\n";
  }

  Textifier tf;
  const int markup_len = input.size();
  char* plaintext = new char[markup_len+1];
  try {
    cout << tf.textify(input.c_str(), markup_len, plaintext, markup_len) << endl;
  }
  catch(string err) {
    long line;
    long column;
    tf.find_location(line, column);
    cerr << "ERROR (" << line << ":" << column << ")  " << err
	 << " at: " << tf.get_snippet() << endl;
    return 1;
  } 
  delete plaintext;
  return 0;
}
