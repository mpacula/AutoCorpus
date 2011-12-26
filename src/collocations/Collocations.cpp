/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

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
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "utilities.h"
#include "merge.h"

using namespace std;
using namespace boost;

const size_t buf_size = 1024*1024;

struct {
  string filePath;
  bool verbose;
  size_t splitSize;
  unsigned int numThreads;
} options;

struct {
  size_t totalSplits;
  size_t splitsComplete;
  size_t totalMerges;
  size_t mergesScheduled;
  size_t mergesComplete;
  timespec startTime;
  mutex mtx;

  bool splitsDone()
  {
    return splitsComplete >= totalSplits;
  }

  bool mergesDone()
  {
    return mergesComplete >= totalMerges;
  }

  bool done()
  {
    return splitsDone() && mergesDone();
  }
} progress;

#define VERBOSE(code) if(options.verbose) { code; }

struct CountPair {
  string w;
  string v;
  long count;  

  CountPair(string w, string v, long count) : w(w), v(v), count(count) { }
  CountPair() : w(""), v(""), count(0) { }

  /// Converts this count pair to the output format.
  void serialize(string& str)
  {
    ostringstream oss;
    oss << count << '\t' << w << " " << v;
    str = oss.str();
  }

  bool operator < (const CountPair& cp) const
  {
    return w < cp.w || (w == cp.w && v < cp.v);
  }
};

struct FileSplit
{
  size_t start;
  size_t end;
  size_t numSplits;

  FileSplit() {}
  FileSplit(size_t start, size_t end, size_t numSplits)
    : start(start), end(end), numSplits(numSplits) { }

  double sizeMB()
  {
    return ((double)(end - start) / pow(1024,2));
  }

  string str() {
    ostringstream os;
    os << "[" << start << ", " << end << "): " << setiosflags(ios::fixed) << setprecision(2) << sizeMB() << "MB";
    return os.str();
  }
};

/// Splits a sentence into words (tokens separated by one or more
/// spaces).
void words(string& str, vector<string>& vec) 
{
  size_t start = 0;
  for(size_t i = 0; i < str.size(); i++) {
    if(str[i] == ' ') {
      if(i > start) {
        vec.push_back(str.substr(start, i-start));        
      } 
      start = i+1;
    }
  }

  if(start < str.size()) {
    vec.push_back(str.substr(start, str.size()));
  }
}

void copyStream(FILE* in, FILE* out)
{
  const size_t buf_size = 10*1024*1024;
  char* buf = new char[buf_size];
  while(!feof(in)) {
    size_t count = fread(buf, 1, buf_size, in);
    fwrite(buf, 1, count, out);
  }
  delete[] buf;
}

double fileSizeGB(FILE* f)
{
  struct stat info;
  fstat(fileno(f), &info);
  return (double)info.st_size / (1024*1024*1024);
}

void reportSplitDone(FILE* result)
{
  progress.mtx.lock();

  progress.splitsComplete++;
  unsigned int h, m, s;
  eta(progress.startTime, progress.splitsComplete, progress.totalSplits, 
      &h, &m, &s);
  
  VERBOSE(fprintf(stderr, "%lu/%lu (%.2f%%) splits complete. output: %.2fGB. eta: %uh %02um %02us\n",
                  progress.splitsComplete, progress.totalSplits,
                  100.0 * progress.splitsComplete / progress.totalSplits,
                  fileSizeGB(result),
                  h, m, s));

  progress.mtx.unlock();
} 

void reportMergeDone(FILE* result)
{
  progress.mtx.lock();

  progress.mergesComplete++;
  unsigned int h, m, s;
  eta(progress.startTime, progress.mergesComplete, progress.totalMerges, 
      &h, &m, &s);
  
  VERBOSE(fprintf(stderr, "%lu/%lu (%.2f%%) merges complete. output: %.2fGB. eta: %uh %02um %02us\n",
                  progress.mergesComplete, progress.totalMerges,
                  100.0 * progress.mergesComplete / progress.totalMerges,
                  fileSizeGB(result),
                  h, m, s));

  progress.mtx.unlock();
} 

/// Destructively Removes duplicates from a vector.
template<class T> void nub(vector<T>& vec) 
{
  unordered_set<T> hs;
  hs.insert(vec.begin(), vec.end());
  vec.assign(hs.begin(), hs.end());
}

/// Emits colocation counts for a words in a single sentence given its
/// context (sentences around it).
void sentenceCounts(vector<string>& sentence, vector<vector<string>>& context,
                    vector<pair<string, string>>& counts) 
{
  // Get all words in the context as a unique set 
  vector<string> ctxWords;
  for(auto ctxSentenceWords : context) {
    ctxWords.insert(ctxWords.end(), ctxSentenceWords.begin(), ctxSentenceWords.end());
  }
  nub(ctxWords);
    
  // emit the counts
  for(string& a : sentence) {
    for(string& b : ctxWords) {
      counts.push_back(make_pair(a, b));
    }
  }
}

/// Emits counts for a single paragraph starting at the given
/// index. Returns the index of the first sentence of the next
/// paragraph or an index after the end of the list if there are no
/// paragraphs left.
void paragraphCounts(unordered_map<string, unordered_map<string, long>>& ht,
                    vector<string>& lines)
{  
  size_t offset = 0;

  // skip empty lines (if any)
  while(offset < lines.size() && trim_copy(lines[offset]) == "") { offset++; }

  // get lines for current paragraph. We add two sentinel empty sentences
  // at the beginning and end so that we always have a sentence before
  // and after current sentence.
  vector<vector<string>> paragraph;
  paragraph.push_back(vector<string>());
  while(offset < lines.size()) {
    paragraph.resize(paragraph.size()+1);
    vector<string>* lineWords = &paragraph[paragraph.size()-1];
    words(lines[offset], *lineWords);
    offset++;
  }
  paragraph.push_back(vector<string>());
 
  // count the colocations in the paragraph
  vector<vector<string>> context;
  vector<pair<string, string>> counts;
  for(size_t i = 1; i < paragraph.size() - 1; i++) {
    context.clear();
    context.push_back(paragraph[i-1]);
    context.push_back(paragraph[i]);
    context.push_back(paragraph[i+1]);

    sentenceCounts(paragraph[i], context, counts);
  }

  // aggregate the counts
  for(auto cp : counts) {
    ht[cp.first][cp.second] += 1;
  }
}

/// Counts colocations for a single split and saves the result in a
/// temporary file, whose handle is returned.
FILE* splitCounts(const char* filePath, FileSplit split)
{
  ifstream file(filePath);
  file.seekg(split.start);
  vector<string> paragraph;
  unordered_map<string, unordered_map<string, long>> ht;
  string line;
  while(file.good() && (size_t)file.tellg() < split.end) {
    getline(file, line);
    if(line == "") {
      // end of paragraph
      paragraphCounts(ht, paragraph);
      paragraph.clear();
      continue;
    }
    paragraph.push_back(line);
  }

  paragraphCounts(ht, paragraph);  
  file.close();

  vector<CountPair> counts;
  size_t i = 0;
  for(auto kv1 : ht) {
    for(auto kv2 : kv1.second) {
      counts.resize(i+1);
      counts[i].w = kv1.first;
      counts[i].v = kv2.first;
      counts[i].count = kv2.second;
      i++;
    }
  }

  sort(counts.begin(), counts.end());

  FILE* out = tmpfile();
  for(auto cp : counts) {
    fprintf(out, "%ld%c%s %s\n", cp.count, COUNT_SEPARATOR, cp.w.c_str(), cp.v.c_str());
  }
  rewind(out);

  reportSplitDone(out);
  return out;
}

size_t seekToParagraph(FILE* f, size_t offset)
{
  fseek(f, offset, SEEK_SET);
  // offset might be between two newlines of a pagraph boundary, in
  // which case our forward-looking method will only see one newline
  // and will fail to break the paragraph. That's ok, as we will
  // simply break after next paragraph Since splits are usually
  // hundreds of megabytes in size, a few kilobatyes extra per split
  // are fine.
  char buf[buf_size];
  char lastChar = '\0';
  while(!feof(f)) {
    int cRead = fread(buf, 1, buf_size, f);
    for(int i = 0; i < cRead; i++) {
      if(lastChar == '\n' && buf[i] == lastChar) {
        // end of paragraph
        fseek(f, offset + i, SEEK_SET);
        return ftell(f);
      }

      lastChar = buf[i];
    }

    // We've scanned the buffer and no paragraph end. Read more data
    // from file.
  }

  // we've reached end of file. Return the current position as end of paragraph
  return ftell(f);
}

/// Finds split points at paragraph boundaries, with each split of
/// size approximately maxSize
void findSplitPoints(FILE* f, size_t maxSize, vector<size_t>& splitPoints) 
{
  // Compute approximate split points, then move them forward to align
  // to paragraph boundaries. Finally sort them and get rid of any
  // duplicates.
  struct stat info;
  fstat(fileno(f), &info);
  size_t fileSize = info.st_size;
  splitPoints.clear();
  double numSplits = ceil((double)fileSize / maxSize);
  for(double i = 1; i < numSplits; i++) {
    size_t estimatedSplitIndex = (long)(i * fileSize / numSplits);
    // align to paragraph
    size_t splitIndex = seekToParagraph(f, estimatedSplitIndex);
    splitPoints.push_back(splitIndex);   
  }
  splitPoints.insert(splitPoints.begin(), 0);
  splitPoints.push_back(fileSize);

  // This is mostly unnecessary. However, for extremely small splits
  // and/or small files the split indices might overlap.
  nub(splitPoints);
  sort(splitPoints.begin(), splitPoints.end());
}

void splitFile(FILE* f, size_t maxSize, vector<FileSplit>& splits)
{
  size_t initialPos = ftell(f);
  vector<size_t> ends;
  findSplitPoints(f, maxSize, ends);
  VERBOSE(cerr << "Total splits: " << ends.size()-1 << endl)

  for(size_t i = 0; i < ends.size()-1; i++) {
    FileSplit split(ends[i], ends[i+1], ends.size()-1);
    splits.push_back(split);
    VERBOSE(cerr << "  - " << split.str() << endl);
  }

  fseek(f, initialPos, SEEK_SET);
}

/// Stores pending counting tasks for splits
queue<FileSplit> splitQueue;
mutex splitMutex;

/// Stores pending merge counts
queue<FILE*> mergeQueue;
mutex mergeMutex;

/// Gets next available file split and removed it from the queue.
/// Thread-safe.
FileSplit* claimSplit(FileSplit& split)
{
  splitMutex.lock();
  if(splitQueue.empty()) {
    splitMutex.unlock();
    return NULL;
  }

  // at least one element present
  split = splitQueue.front();
  splitQueue.pop();
  splitMutex.unlock();
  return &split;
}

void scheduleSplit(FileSplit split) 
{
  splitMutex.lock();
  splitQueue.push(split);
  splitMutex.unlock();
}

void scheduleMerge(FILE* file) 
{
  mergeMutex.lock();
  mergeQueue.push(file);
  mergeMutex.unlock();
}

bool claimMerge(FILE*& f1, FILE*& f2)
{
  mergeMutex.lock();
  if(mergeQueue.size() < 2) {
    mergeMutex.unlock();
    return false;
  }
  
  // we have at least two files to merge
  f1 = mergeQueue.front();
  mergeQueue.pop();
  f2 = mergeQueue.front();
  mergeQueue.pop();

  mergeMutex.unlock();
  return true;
}

/// Takes splits off the split queue and counts collocations in the
/// split
void splitTask() 
{
  while(!progress.done()) {
    FileSplit split;
    // we check for small mergeQueue size so that we don't generate
    // too many splits that go unmerged. This is to preserve disk
    // space.
    if(mergeQueue.size() < 5 && claimSplit(split)) {
      // we got ourselves a split task
      FILE* out = splitCounts(options.filePath.c_str(), split);
      scheduleMerge(out);
      continue;
    }

    // wait for splits
    sleep(1);
  }

  VERBOSE(cerr << "Split worker thread terminating." << endl);
}

void mergeTask() 
{
  while(!progress.done()) {
    FILE* f1, *f2;
    if(claimMerge(f1, f2)) {
      // we got a merge task!
      FILE* out = tmpfile();
      mergeCounts(f1, f2, out);
      reportMergeDone(out);
      fclose(f1);
      fclose(f2);
      scheduleMerge(out);
      continue;
    }
    
    // wait for merges
    sleep(1);
  }

  VERBOSE(cerr << "Merge worker thread terminating." << endl);
}

int getRequiredMergeCount(unsigned int n)
{
  unsigned int c = 0;
  while(n > 1)
    {
      c += floor((double)n/2);
      n = ceil((double)n/2);
    }
  return c;
}

/// Counts collocations in the given file, outputting the results to
/// stdout.
int countCollocations()
{
  FILE* inputFile = fopen(options.filePath.c_str(), "r");
  if(inputFile == NULL) {
    cerr << "Could not open file " << options.filePath << endl;
    return 1;
  }

  vector<FileSplit> splits;
  VERBOSE(cerr << "Computing splits..." << endl);
  splitFile(inputFile, options.splitSize, splits);
  progress.totalSplits = splits.size();
  progress.totalMerges = getRequiredMergeCount(progress.totalSplits);

  for(auto split : splits) {
    scheduleSplit(split);
  }

  VERBOSE(cerr << "Counting..." << endl);
  clock_gettime(CLOCK_MONOTONIC, &progress.startTime);
  
  vector<thread*> workers;
  for(unsigned int i = 0; i < options.numThreads; i++) {
    // merge tasks are IO bound and split tasks are CPU bound. It's ok
    // to use separate threads for the merge tasks without slowing
    // down counting in splits.
    thread* worker = new thread(mergeTask);
    workers.push_back(worker);

    worker = new thread(splitTask);
    workers.push_back(worker);
  }

  for(auto worker : workers) {
    worker->join();
    delete worker;
  }

  if(mergeQueue.size() != 1) {
    cerr << "Merge queue empty or too many results (" << mergeQueue.size() << "). This is a bug." << endl;

    while(mergeQueue.size() > 0) {
      fclose(mergeQueue.front());
      mergeQueue.pop();
    }
    return 1;
  }

  VERBOSE(cerr << "Writing results to stdout." << endl);
  copyStream(mergeQueue.front(), stdout);
  fclose(mergeQueue.front());
  mergeQueue.pop();

  fclose(inputFile);
  return 0;
}

void printUsage(const char* name)
{
  printf("Usage: %s [-m LIMIT] [-v] [-t THREADS] file\n", name);
}

int main(int argc, char* argv[])
{
  // Default options
  options.verbose = false;
  options.splitSize = 1*1024*1024;
  options.numThreads = 1;

  // Parse options
  vector<string> unparsedOpts;
  for(int i=1; i<argc; i++) {
    if(strcmp("-m", argv[i]) == 0 && i<argc-1) {
      long multiplier = 1;
      char unit = '\0';
      sscanf(argv[i+1], "%ld%c", &options.splitSize, &unit);
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
      
      options.splitSize *= multiplier;
      i++;
    }
    else if(strcmp("-v", argv[i]) == 0) {
      options.verbose = true;
    }
    else if(strcmp("-t", argv[i]) == 0) {
      sscanf(argv[i+1], "%d", &options.numThreads);
      i++;
    }
    else {
      unparsedOpts.push_back(argv[i]);
    }
  }

  // the only argument left should be the filename
  if(unparsedOpts.size() != 1) {
    printUsage(argv[0]);
    return 1;
  }

  options.filePath = unparsedOpts[0];
  return countCollocations();
}
