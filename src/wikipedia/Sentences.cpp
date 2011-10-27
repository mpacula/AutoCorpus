#include <iostream>
#include <string>

#include "SentenceExtractor.h"

using namespace std;

int main(int argc, char** argv)
{
  if(argc != 1) {
    cerr << "Usage: " << argv[0] << " <stdin>" << endl;
    return 1;
  }
  
  ExtractorOptions opts;
  opts.separateParagraphs = true;
  SentenceExtractor extractor(opts);
  string input;
  string line;
  while(getline(cin, line)) {
    if(line == "\f") {
        string output = extractor.extract(input.c_str());
        cout << output << endl;
        cout << "\f" << endl;
        input.clear();
    }
    else {
      input += line;
      input += "\n";
    }
  }  

  if(input.length() > 0) {
    string output = extractor.extract(input.c_str());
    cout << output << endl;
    cout << "\f" << endl;
    input.clear();
  }
}
