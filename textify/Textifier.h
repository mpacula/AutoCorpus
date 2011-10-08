#ifndef Textifier_h
#define Textifier_h

#include <pcre.h>
#include <string>

typedef struct _State
{
  size_t           N;          // input length
  size_t           pos;        // current position within input
  const char*      markup;     // the markup input we're converting
  char*            out;        // output std::string
  size_t           M;          // maximum length of output without the terminating \0
  size_t           pos_out;    // position within output std::string
  std::string      groups[10]; // stores regexp matches
} State;

class Textifier 
{
private:
  State state;

  bool starts_with(std::string& str);
  bool starts_with(const char* str);
  const char* get_remaining();
  char* get_current_out();
  void skip_match();
  void skip_line();
  void append_group_and_skip(int group);

  void do_link();
  void do_format();
  void do_tag();
  void do_meta_box();
  void do_meta_pipe();
  void do_heading();
  void do_comment();
  void do_list();
  void ignore_nested(std::string name, char open, char close);

  bool get_link_boundaries(int& start, int& end, int& next);

  bool at_line_start(const char* str, int pos);

  std::string get_err(std::string name);
  std::string* match(std::string name, pcre* regexp);

  pcre* make_pcre(const char* expr, int options);
  pcre* re_format;
  pcre* re_heading;
  pcre* re_comment;

public:
  Textifier();
  ~Textifier();
  char* textify(const char* markup, const int markup_len,
                char* out, const int out_len);

  void find_location(long& line, long& col);
  std::string get_snippet();
};

#endif // Textifier_h
