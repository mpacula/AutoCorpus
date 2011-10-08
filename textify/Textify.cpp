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
