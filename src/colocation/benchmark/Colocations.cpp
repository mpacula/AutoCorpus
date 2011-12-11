#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace boost;

struct CountPair {
  string wv;
  long count;  

  CountPair(string wv, long count) : wv(wv), count(count) { }

  /// Converts this count pair to the output format.
  void serialize(string& str)
  {
    ostringstream oss;
    oss << count << '\t' << wv;
    str = oss.str();
  }

  bool operator < (const CountPair& cp) const
  {
    return wv < cp.wv;
  }
};

/// Splits a sentence into words (tokens separated by one or more
/// spaces).
void words(string& str, vector<string>& vec) 
{
  split(vec, str, is_any_of(" "), token_compress_on);
  // remove empty tokens
  for(size_t i = 0; i < vec.size(); i++) {
    if(vec[i] == "") {
      vec.erase(vec.begin() + i);
      i--;
      continue;
    }
  }
}

/// Destructively Removes duplicates from a vector.
void nub(vector<string>& vec) 
{
  sort(vec.begin(), vec.end());
  vec.erase(unique(vec.begin(), vec.end()), vec.end());
}

/// Coverts a string into a vector of unique words.
void sentenceToSet(string& str, vector<string>& vec) 
{
  words(str, vec);
  nub(vec);
}

/// Emits colocation counts for a words in a single sentence given its
/// context (sentences around it).
void sentenceCounts(string& str, vector<string>& context,
                    vector<CountPair>& counts) 
{
  // get words in the sentence of interest
  vector<string> sentenceWords;
  words(str, sentenceWords);

  // Get all words in the context as a unique set 
  string ctx;
  for(string& sentence : context) {
    ctx += sentence + " ";
  }
  vector<string> ctxWords;
  sentenceToSet(ctx, ctxWords);
  
  // emit the counts
  for(string& a : sentenceWords) {
    for(string& b : ctxWords) {
      counts.push_back(CountPair(a+" "+b,1));
    }
  }
}

/// Emits counts for a single paragraph starting at the given
/// index. Returns the index of the first sentence of the next
/// paragraph or an index after the end of the list if there are no
/// paragraphs left.
int paragraphCounts(unordered_map<string, long>& ht, vector<string>& lines, size_t offset) 
{  
  // skip empty lines (if any)
  while(offset < lines.size() && trim_copy(lines[offset]) == "") { offset++; }

  // get lines for current paragraph. We add two sentinel empty sentences
  // at the beginning and end so that we always have a sentence before
  // and after current sentence.
  vector<string> paragraph;
  paragraph.push_back("");
  while(offset < lines.size() && trim_copy(lines[offset]) != "") {
      paragraph.push_back(lines[offset++]);
  }
  paragraph.push_back("");
 
  // count the colocations in the paragraph
  vector<string> context;
  vector<CountPair> counts;
  for(size_t i = 1; i < paragraph.size() - 1; i++) {
    context.clear();
    context.push_back(paragraph[i-1]);
    context.push_back(paragraph[i+1]);

    sentenceCounts(paragraph[i], context, counts);
  }

  // aggregate the counts
  for(CountPair& cp : counts) {
    ht[cp.wv] += cp.count;
  }

  return offset;
}

/// Counts colocations for a document, i.e. a set of paragraphs
/// separated by one or more empty lines.
void documentCounts(vector<string>& lines, vector<CountPair>& counts)
{
  /// Accumulate counts in a hash map one paragraph at a time and then
  /// return sored counts in a vector..
  size_t offset = 0;
  unordered_map<string, long> ht;
  while(offset < lines.size()) {
    offset = paragraphCounts(ht, lines, offset);
  }

  for(auto kv : ht) {
    counts.push_back(CountPair(kv.first, kv.second));
  }

  sort(counts.begin(), counts.end());
}

int main(int argc, char* argv[])
{
  // Read all lines from stdin, call documentCounts, print resulting
  // counts to stdout and quit.
  vector<string> lines;
  string line;
  while(getline(cin, line)) {
    lines.push_back(line);
  }

  vector<CountPair> counts;
  documentCounts(lines, counts);
  for(auto cp : counts) {
    cout << cp.count << "\t" << cp.wv << endl;
  }

  return 0;
}
