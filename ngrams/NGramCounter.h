#ifndef NGramCounter_h
#define NGramCounter_h

#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

/** Iteratively counts n-grams in input, line by line. */
class NGramCounter
{
 private:
  std::unordered_map<std::string, long> currentCounts;
  std::set<std::string> sortedNGrams;
  std::vector<std::string> lineNGrams;
  int n; // desired number of words in an ngram
  bool closed;  
  long totalCount;
  size_t maxChunkSize; // in bytes, including auxiliary data structures
  size_t maxChunkLength; // in characters, w/o auxiliary data structures
  size_t chunkLength;

  std::vector<FILE*> chunkFiles; // store chunk filenames
  void endChunk();
  FILE* mergeChunks(FILE*, FILE*, bool);
  void mergeAll();
  void printOnlyChunk();

  /** Returns ngrams in a single line of input (may return
      duplicates if they exist) */
  std::vector<std::string>* ngrams(std::string line, std::vector<std::string>*);
  
 public:
  NGramCounter(const int, const size_t);
  ~NGramCounter();
  void count(std::string line);  
  void close();
};

#endif // NGramCounter_h
