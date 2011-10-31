/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    Sentences.cpp: the front-end to SentenceExtractor - reads input line
                   by line from stdin and splits it into separate
                   sentences. Sentences are outputted to stdout, one
                   per line, with paragraphs separated by one blank line.
    

                   
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
