#include <iostream>
#include <string>

#include "SentenceExtractor.h"

using namespace std;

int main(int argc, char** argv)
{
  string input;
  string line;
  while(getline(cin, line)) {
    input += line;
    input += "\n";
  }
  
  ExtractorOptions opts;
  opts.separateParagraphs = true;
  SentenceExtractor extractor(opts);

  string output = extractor.extract(input.c_str());
  cout << output << endl;
}
