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
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

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

/// Destructively Removes duplicates from a vector.
void nub(vector<string>& vec) 
{
  unordered_set<string> hs;
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
int paragraphCounts(unordered_map<string, unordered_map<string, long>>& ht,
                    vector<string>& lines, size_t offset) 
{  
  // skip empty lines (if any)
  while(offset < lines.size() && trim_copy(lines[offset]) == "") { offset++; }

  // get lines for current paragraph. We add two sentinel empty sentences
  // at the beginning and end so that we always have a sentence before
  // and after current sentence.
  vector<vector<string>> paragraph;
  paragraph.push_back(vector<string>());
  while(offset < lines.size() && trim_copy(lines[offset]) != "") {
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
    context.push_back(paragraph[i+1]);

    sentenceCounts(paragraph[i], context, counts);
  }

  // aggregate the counts
  for(auto cp : counts) {
    ht[cp.first][cp.second] += 1;
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
  unordered_map<string, unordered_map<string, long>> ht;
  while(offset < lines.size()) {
    offset = paragraphCounts(ht, lines, offset);
  }

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
    cout << cp.count << "\t" << cp.w << " " << cp.v << endl;
  }

  return 0;
}
