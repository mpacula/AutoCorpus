/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    NGramCounter.cpp: implements a constant-memory ngram counter. Input
                      is read from stdin with words assumed to be separated
                      by spaces, and output us written to stdout. The first
                      line of output always contains the sum of all counts.
                      Subsequent lines list the actual counts in the format:
                      count\tngram, i.e. counts followed by a tab character
                      followed by the ngram.

                      NGramCounter uses a disk cache to store intermediate
                      results. The intermediate results are stored in 
                      temporary files called "chunks", which NGramCounter
                      periodically merges in a manner similar to a 2-way
                      merge sort.
    


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
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include "string.h"
#include "NGramCounter.h"
#include "utilities.h"
#include "merge.h"


using namespace std;

bool isAllWhitespace(string str)
{
  for(size_t i=0; i<str.size(); i++) {
    char ch = str[i];
    if(ch != ' ' && ch != '\n' && ch != '\r' && ch != '\f' && ch != '\t')
      return false;
  }
  return true;
}

NGramCounter::NGramCounter(const int n, const size_t maxChunkSize, const bool verbose) {
  this->n = n;
  this->closed = false;
  this->maxChunkSize = maxChunkSize;

  // an approximation of how many bytes of input
  // we can store in a chunk without going over maxChunkSize
  // Disclaimer: 2n because we store each ngram twice: as a key in
  // the hashmap and as a member in the sorted set. 8 because
  // we store a long count for each. The constant '4' is empirical.
  this->maxChunkLength = 4*maxChunkSize / (2*n+8);
  this->chunkLength = 0;
  this->currentCounts.rehash(this->maxChunkLength / 6);
  this->totalCount = 0;
  this->verbose = verbose;
}

NGramCounter::~NGramCounter()
{
}

void NGramCounter::endChunk()
{
  // Create chunk file and write out the ngrams
  FILE* chunkFile = tmpfile();
  if(chunkFile == NULL)
    throw string("ERROR: could not create chunk file. Possibly too many chunks? Try increasing chunk size.");
  
  for(set<string>::iterator it = sortedNGrams.begin(); it != sortedNGrams.end(); it++) {
    string ngram = *it;
    long count = currentCounts[ngram];
    fprintf(chunkFile, "%ld%c%s\n", count, NGRAM_SEPARATOR, ngram.c_str());
  }  
  fflush(chunkFile);
  rewind(chunkFile);
  
  // Clean state for next chunk
  sortedNGrams.clear();  
  currentCounts.clear();
  chunkLength = 0;
  
  chunkFiles.push_back(chunkFile);

  if(chunkFiles.size() >= 3) {
    // This ensures we have at most 3 chunks at any time.
    // This is because an excessive number of chunks can strain
    // system resources, and calls to tmpfile might fail.
    FILE* merged = mergeChunks(chunkFiles[0], chunkFiles[1], false);
    chunkFiles.erase(chunkFiles.begin()+1);
    chunkFiles.erase(chunkFiles.begin());
    chunkFiles.push_back(merged);
  }
}

void NGramCounter::count(const string line)
{
  if(closed)
    throw string("NGramCounter is closed.");
  else if(isAllWhitespace(line))
    return;

  ngrams(line, &lineNGrams);
  for(vector<string>::iterator it = lineNGrams.begin(); it < lineNGrams.end(); it++) {
    currentCounts[*it] += 1;
    sortedNGrams.insert(*it);
    totalCount++;
  }

  chunkLength += (line.length()+1); // +1 for the newline
  if(chunkLength > maxChunkLength) {
    endChunk();
  }
}

void NGramCounter::printOnlyChunk()
{
  if(chunkFiles.size() != 1)
    throw string("printOnlyChunk: invalid number of chunks");

  const size_t buf_size = 10*1024*1024; // 10MB
  char* buf = new char[buf_size];
  char* ngram = new char[buf_size];
  printf("%ld\n", totalCount); // total no. of ngrams

  long c = 0;
  long c_total = 0;
  while(fgets(buf, buf_size, chunkFiles[0])) {
    cout << buf;
    deconstructCount(buf, ngram, &c);
    c_total += c;
  }

  if(!feof(chunkFiles[0]))
    throw string("printOnlyChunk: read error");

  if(c_total != totalCount) {
    cerr << "WARNING: input and output ngram counts mismatch: " << totalCount << " vs. " << c_total << endl;
  }
  
  delete[] buf;
  delete[] ngram;
  fclose(chunkFiles[0]);
  chunkFiles.clear();
}


FILE* NGramCounter::mergeChunks(FILE* chunk1, FILE* chunk2, bool last)
{
  FILE* mergedFile;
  if(last) {
    printf("%ld\n", totalCount);
    mergedFile = stdout;
  }
  else
    mergedFile = tmpfile();

  if(mergedFile == NULL)
    throw string("Could not create output file to store merged chunks.");

  mergeCounts(chunk1, chunk2, mergedFile);

  // mergedFile is yet another chunk that we'll have to merge. Hence we'll rewind it
  // so that future reads start from the beginning.
  if(!last) 
    rewind(mergedFile);
  
  fclose(chunk1);
  fclose(chunk2);
  return last ? NULL : mergedFile;
}

void NGramCounter::mergeAll()
{
  if(chunkFiles.size() == 1) {
    printOnlyChunk();
    return;
  }  

  // pairwise merge until two chunks remain
  while(chunkFiles.size() > 2) {
    if(verbose)
      cerr << "Merge one level. Start no. chunks: " << chunkFiles.size() << endl;

    vector<FILE*> newChunks;
    newChunks.reserve(chunkFiles.size());
    for(size_t i=0; i<chunkFiles.size()-1; i += 2) {
      FILE* chunk1 = chunkFiles[i];
      FILE* chunk2 = chunkFiles[i+1];
      newChunks.push_back(mergeChunks(chunk1, chunk2, false));    
    }

    if(chunkFiles.size() % 2 == 1) // odd number of chunks
      newChunks.push_back(chunkFiles[chunkFiles.size()-1]);

    chunkFiles = newChunks;
  }

  // At this point we only have two chunks: just print them out,
  // no reason to save them to a file first since we're done.
  mergeChunks(chunkFiles[0], chunkFiles[1], true);
  chunkFiles.clear();
}

void NGramCounter::close()
{
  endChunk();
  mergeAll();
  closed = true;
}

vector<string>* NGramCounter::ngrams(const string line, vector<string>* ngrams)
{
  vector<string> words;
  string word = "";

  // add "start" words
  for(int i=0; i < n-1; i++) {
    words.push_back("<s>");
  }

  for(size_t i=0; i<line.size(); i++) {
    char ch = line[i];
    if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
      if(word.size() > 0)
        words.push_back(word);

      word = "";
    }
    else
      word += ch;
  }

  if(word.size() > 0)
    words.push_back(word);

  words.push_back("</s>"); // the "end" word

  // build ngrams based on tokenized line
  ngrams->clear();
  for(size_t i=n-1; i<words.size(); i++) {
    string ngram = "";
    for(size_t j=i-n+1; j <= i; j++) {
      if(ngram.size() > 0)
        ngram += " ";
      ngram += words[j];
    }
    ngrams->push_back(ngram);
  }
  return ngrams;
}

void printUsage(const char* name)
{
  printf("Usage: %s [-n NUMBER] [-m LIMIT] [-v]\n", name);
}

int main(int argc, const char** argv)
{
  size_t chunkSize = 500*1024*1024;
  int n = 2;
  bool verbose = false;
  for(int i=1; i<argc; i++) {
    if(strcmp("-m", argv[i]) == 0 && i<argc-1) {
      long size = 100*1024*1024;
      long multiplier = 1;
      char unit = '\0';
      sscanf(argv[i+1], "%ld%c", &size, &unit);
      unit = tolower(unit);

      switch(unit) {
      case 'b':
        multiplier = 512;
        break;
      case 'k':
        multiplier = 1024;
        break;
      case 'm':
        multiplier = 1024*1024;
        break;
      case 'g':
        multiplier = 1024*1024*1024;
        break;
      case'\0':
        multiplier = 1;
        break;
      default:
        printUsage(argv[0]);
        return 1;
      }
      
      chunkSize = size*multiplier;
      i++;
    }
    else if(strcmp("-n", argv[i]) == 0 && i<argc-1) {
      n = atoi(argv[i+1]);
      i++;
    }
    else if(strcmp("-v", argv[i]) == 0) {
      verbose = true;
    }
    else {
      printUsage(argv[0]);
      return 1;
    }
  }

  if(chunkSize < 10*1024*1024) {
    cerr << "WARNING: Very small selected chunk size. Performance might suffer." << endl;
  }

  if(n <= 0) {
    cerr << "Invalid ngram size: " << n << endl;
    return 1;
  }

  NGramCounter counter(n, chunkSize, verbose);

  string line;
  try {
    while(getline(cin, line)) {
      counter.count(line);
    }
    counter.close();
  } catch(string err) {
    cerr << err << endl;
    return 1;
  }
}
