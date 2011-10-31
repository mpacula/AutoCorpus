/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    NGramCounter.h: see NGramCounter.cpp for a description.


    
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
  bool verbose;
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
  NGramCounter(const int, const size_t, const bool);
  ~NGramCounter();
  void count(std::string line);  
  void close();
};

#endif // NGramCounter_h
