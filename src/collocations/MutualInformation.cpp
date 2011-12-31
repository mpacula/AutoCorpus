#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>

#include "utilities.h"

using namespace std;

/* TODO
    - verbose switch
    - perf optimizations
*/

struct 
{
  char* unigramsPath;
  long countCutoff;
} options;

bool loadUnigrams(unordered_map<string, long>& ht, long& total)
{
  cerr << "Loading unigrams... ";
  string line;
  long count;
  char ngram[1024*1024];
  ifstream file(options.unigramsPath);

  if(!file.good()) {
    fprintf(stderr, "Error reading file %s\n", options.unigramsPath);
    return false;
  }

  getline(file, line);
  if(sscanf(line.c_str(), "%ld", &total) < 1) {
    fprintf(stderr, "Could not parse total number of unigrams. Line: %s\n", line.c_str());
    return 1;
  }

  long errors = 0;
  while(!file.eof()) {
    getline(file, line);
    if(!deconstructCount(line.c_str(), ngram, &count)) {
      errors++;
      continue;
    }
    ht[ngram]+=count;
  }
  cerr << "ok. " << ht.size() << " unique. " << errors << " errors." << endl;
  file.close();
  return true;
}

void printMI(string& currentWord, unordered_map<string, long>& counts,
             unordered_map<string, long>& unigrams, long N)
{
  if(unigrams.count(currentWord) == 0) {
    //cerr << "Warning: unigram '" << currentWord << "' does not exist (w)." << endl;
    return;
  }

  double C_w = unigrams[currentWord];
  if(C_w < options.countCutoff)
    return;

  // Covert raw collocation counts to mututal information
  unordered_map<string, double> mi;
  vector<string> vs;
  for(auto collocationPair : counts) {
    const string& v = collocationPair.first;
    double C_wv = collocationPair.second;
    if(unigrams.count(v) == 0) {
      cerr << "Warning: unigram '" << v << "' does not exist (v)." << endl;
      continue;
    }

    double C_v = unigrams[v];
    if(C_v < options.countCutoff)
      continue;

    mi[v] = C_wv*N*N / (C_w*C_w*C_v);
    vs.push_back(v);
  }

  double norm = mi[currentWord];
  // sort by decreasing MI
  sort(vs.begin(), vs.end(), 
       [&](const string& a, const string& b) { return mi[a] > mi[b]; });
  for(auto v : vs) {
    const double mi_v = mi[v];
    cout << mi_v/norm << COUNT_SEPARATOR 
         << mi_v << COUNT_SEPARATOR 
         << counts[v] << COUNT_SEPARATOR 
         << currentWord << COUNT_SEPARATOR
         << v << endl;
  }
}

bool computeMI(unordered_map<string, long>& unigrams, long N)
{
  string line;  
  string currentWord;
  unordered_map<string, long> counts;
  vector<string> keyWords;
  char key[1024*1024];

  while(cin.good())
  {
    getline(cin, line);
    long c;
    if(!deconstructCount(line.c_str(), key, &c)) {
      //fprintf(stderr, "\nWarning: could not parse count for line: %s\n", line.c_str());
      continue;
    }

    keyWords.clear();
    words(key, keyWords);
    if(keyWords.size() != 2) {
      //cerr << "Warning: invalid number of words ("
      //     << keyWords.size() << ", expected 2): " << key << endl;
      continue;
    }

    if(keyWords[0] != currentWord) {
      if(currentWord != "")
        printMI(currentWord, counts, unigrams, N);
      counts.clear();
      currentWord = keyWords[0];      
    }

    counts[keyWords[1]] = c;
  }

  if(currentWord != "" && counts.size() > 0)
    printMI(currentWord, counts, unigrams, N);
  return true;
}

void printUsage(const char* name)
{
   printf("Usage: %s --unigrams FILE\n", name);
}

int main(int argc, char** argv)
{
  // Default options
  options.unigramsPath = NULL;
  options.countCutoff = 0;

  for(int i=1; i<argc; i++) {
    bool more = i < argc-1;
    char* arg = argv[i];
    if(strcmp(arg, "--unigrams") == 0 && more) {
      options.unigramsPath = argv[i+1];
      i++;
    }
    else if(strcmp(arg, "-ct") == 0 && more) {
      sscanf(argv[i+1], "%ld", &options.countCutoff);
      i++;
    }   
    else {
      printUsage(argv[0]);
      return 1;
    }
  }

  if(!options.unigramsPath) {
    printUsage(argv[0]);
    return 1;
  }

  unordered_map<string, long> unigrams;
  long unigramsTotal;
  if(!loadUnigrams(unigrams, unigramsTotal)) {
    return 1;
  }
  
  return computeMI(unigrams, unigramsTotal);
}
