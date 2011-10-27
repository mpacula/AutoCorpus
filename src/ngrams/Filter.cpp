/** Filters out bigrams with low counts */

#include <stdio.h>
#include <iostream>
#include <string>
#include "utilities.h"

using namespace std;

class CountFilter 
{
private:
  long threshold;
  long cAbove; // count of ngrams above threshold
  long cBelow; // count of ngrams below threshold
  long cExpectedTotal; // number of ngrams read from input header
  bool header;
  FILE* tmpFile;
  char* ngram;

public:
  CountFilter(long threshold)
  {
    this->threshold = threshold;    
    tmpFile = tmpfile();
    cBelow = cAbove = cExpectedTotal = 0;
    header = true;
    ngram = new char[1024*1024];
  }

  ~CountFilter()
  {
    delete ngram;
    if(tmpFile != NULL)
      fclose(tmpFile);
  }

  void filter(string line)
  {
    if(header) {
      sscanf(line.c_str(), "%ld", &cExpectedTotal);
      header = false;
      return;
    }
    
    long c = 0;
    if(!deconstructNGram(line.c_str(), ngram, &c)) {
      cerr << "WARNING: could not read ngram from input line:\n" << line << endl;
      return;
    }

    if(c >= threshold) {
      fputs(line.c_str(), tmpFile);
      fputc('\n', tmpFile);
      cAbove += c;
    } else 
      cBelow += c;
  }

  void close() 
  {
    if(cBelow + cAbove != cExpectedTotal) {
      cerr << "WARNING: actual number of ngrams does not match header: " << (cBelow + cAbove)
           << " vs. " << cExpectedTotal << endl;
    }
    fflush(tmpFile);
    rewind(tmpFile);
    printf("%ld\n", cAbove);

    const size_t buf_size = 100*1024*1024;
    char* buf = new char[buf_size];
    while(true) {
      char* read = fgets(buf, buf_size, tmpFile);
      if(read)
        fputs(buf, stdout);
      else
        break;
    }

    if(!feof(tmpFile)) {
      delete[] buf;
      fclose(tmpFile);
      tmpFile = NULL;
      throw string("ERROR: could not read temporary file.");      
    }    
    
    delete[] buf;
    fclose(tmpFile);
    tmpFile = NULL;
  }
};

void printUsage(const char* name)
{
  printf("Usage: %s [-t THRESHOLD]\n", name);
}

int main(int argc, const char** argv)
{
  long threshold = 5;

  for(int i=1; i<argc; i++) {
    if(strcmp("-t", argv[i]) == 0 && i<argc-1) {
      threshold = atoi(argv[i+1]);
      i++;
    }
    else {
      printUsage(argv[0]);
      return 1;
    }
  }

  CountFilter filter = CountFilter(threshold);

  string line;
  try {
    while(getline(cin, line)) {
      filter.filter(line);
    }
    filter.close();
  } catch(string err) {
    cerr << err << endl;
    return 1;
  }

  return 0;
}
