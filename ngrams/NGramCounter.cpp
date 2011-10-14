#include <stdlib.h>
#include <iostream>
#include "string.h"
#include "NGramCounter.h"

#define SEPARATOR '\t'

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

NGramCounter::NGramCounter(const int n, const size_t maxChunkSize) {
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
}

NGramCounter::~NGramCounter()
{
}

void NGramCounter::endChunk()
{
  // Create chunk file and write out the ngrams
  FILE* chunkFile = tmpfile();
  
  for(set<string>::iterator it = sortedNGrams.begin(); it != sortedNGrams.end(); it++) {
    string ngram = *it;
    long count = currentCounts[ngram];
    fprintf(chunkFile, "%s%c%ld\n", ngram.c_str(), SEPARATOR, count);
  }  
  fflush(chunkFile);
  rewind(chunkFile);
  
  // Clean state for next chunk
  sortedNGrams.clear();  
  currentCounts.clear();
  chunkLength = 0;
  
  chunkFiles.push_back(chunkFile);
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
  printf("%ld\n", totalCount); // total no. of ngrams

  while(fgets(buf, buf_size, chunkFiles[0])) {
    cout << buf;
  }

  if(!feof(chunkFiles[0]))
    throw string("printOnlyChunk: read error");
  
  delete[] buf;
  fclose(chunkFiles[0]);
  chunkFiles.clear();
}

char* deconstructNGram(const char* str, char* ngram, long* count) {
  const char* delim = strchr(str, SEPARATOR);
  if(delim == NULL)
    return NULL;

  strncpy(ngram, str, (size_t)(delim-str));
  sscanf(&delim[1], "%ld", count);
  return ngram;
}

FILE* NGramCounter::mergeChunks(FILE* chunk1, FILE* chunk2, bool print)
{
  //cout << "Merging chunks " << chunk1 << " and " << chunk2 << endl;
  FILE* mergedFile;
  if(print)
    mergedFile = stdout;
  else
    mergedFile = tmpfile();

  if(mergedFile == NULL)
    throw string("Could not create output file to store merged chunks.");

  const size_t buf_size = 1024*1024;
  char* line1 = new char[buf_size];
  char* line2 = new char[buf_size];
  char* ngram1 = new char[buf_size];
  char* ngram2 = new char[buf_size];
  long c1, c2;
  while(true) {
    line1 = fgets(line1, buf_size, chunk1);
    line2 = fgets(line2, buf_size, chunk2);

    if(line1 != NULL && line2 != NULL) {
      // add counts and write out      
      if(!deconstructNGram(line1, ngram1, &c1) || !deconstructNGram(line2, ngram2, &c2)) {
	cerr << "Could not deconstruct ngram when merging chunks. Lines (chunk 1 and chunk 2)): " << endl;
	cerr << line1;
	cerr << line2;
      }

      switch(strcmp(ngram1, ngram2)) {
      case 0:
        fprintf(mergedFile, "%s%c%ld\n", ngram1, SEPARATOR, c1+c2);
        break;
      case -1:
        fputs(line1, mergedFile);
        fputs(line2, mergedFile);
        break;;
      case 1:
        fputs(line2, mergedFile);
        fputs(line1, mergedFile);
        break;;
      }
    }
    else if(line1 != NULL) {
      fputs(line1, mergedFile);
    }
    else if(line2 != NULL) {
      fputs(line2, mergedFile);
    }
    else
      break;
  }

  if(!feof(chunk1) || !feof(chunk2))
    throw string("Read error on at least one chunk: read failed before end of file");
  
  if(!print)
    rewind(mergedFile);

  delete[] ngram1;
  delete[] ngram2;
  fclose(chunk1);
  fclose(chunk2);
  return print ? NULL : mergedFile;
}

void NGramCounter::mergeAll()
{
  if(chunkFiles.size() == 1) {
    printOnlyChunk();
    return;
  }
  

  // pairwise merge until two chunks remain
  while(chunkFiles.size() > 2) {
    // cout << "Merge one level. Start no. chunks: " << chunkFiles.size() << endl;

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
    // cout << "End no. chunks: " << chunkFiles.size() << endl;
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

int main(int argc, char** argv)
{
  size_t chunkSizeMBytes = 100;
  for(int i=1; i<argc; i++) {
    if(strcmp("-s", argv[i]) == 0) {
      chunkSizeMBytes = atoi(argv[++i]);
    }
  }
  NGramCounter counter(2, chunkSizeMBytes*1024*1024);

  string line;
  while(getline(cin, line)) {
    try {
      counter.count(line);
    } catch(string err) {
      cerr << err << endl;
      return 1;
    }
  }
  try {
    counter.close();
  } catch(string err) {
    cerr << err << endl;
    return 1;
  }
}
