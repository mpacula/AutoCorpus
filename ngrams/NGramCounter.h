#ifndef NGramCounter_h
#define NGramCounter_h

#include <string>
#include <vector>
#include <unordered_map>

/** Iteratively counts n-grams in input, line by line. */
class NGramCounter
{
 private:
  std::unordered_map<std::string, long> currentCounts;
  
 public:
  void count(std::string line);
};

#endif // NGramCounter_h
