#ifndef SentenceExtractor_h
#define SentenceExtractor_h

#include <string>

typedef struct _ExtractorOptions
{
  bool separateParagraphs; // separate paragraphs with newlines in output?
} ExtractorOptions;

class SentenceExtractor
{
 private:
  const char* input;
  size_t pos;
  std::string output;
  ExtractorOptions opts;

  char last_written_char();
  char peek();
  bool is_last_written_char(const char*);
  bool is_ws(char ch);

 public:
  SentenceExtractor(ExtractorOptions);
  ~SentenceExtractor(); 
  
  std::string extract(const char*);  
};

#endif // SentenceExtractor_h
