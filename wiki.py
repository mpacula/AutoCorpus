import numpy
import sys
import os
import re
import xml.parsers.expat as sax

def err(msg):
  """ Prints a message to stderr, terminating it with a newline """
  sys.stderr.write(msg + "\n")


# A fast, finite-state automaton based plaintext converter for mediawiki markup
class MarkupToTextConverter:
  """ Converts wiki markup to plain text """
  def convert(self, markup):
    self.markup = markup
    self.N = len(markup)
    self.pos = 0
    self.snippets = []

    start = 0
    end = 0
    while self.pos < self.N:
      rest = self.markup[self.pos:]
      pos_prev = self.pos

      special_handler = None
      if rest.startswith("<nowiki>"):
        special_handler = self.do_nowiki
      elif rest.startswith("["):
        special_handler = self.do_link
      elif rest.startswith("<"):
        special_handler = self.do_tag
      elif rest.startswith("{{") or rest.startswith("{|"):
        special_handler = self.do_meta
      elif rest.startswith("=") and self.line_before().strip() == "":
        special_handler = self.do_heading
      elif rest.startswith("''"):
        special_handler = self.do_format

      if special_handler != None:
        # one of the special cases above matched: save current snippet
        self.add_current_snippet(start, end)

        if special_handler != None:
          special_handler()

          # skip past whatever special handler parsed
          start = self.pos
          end = self.pos
      else:
        end += 1
        self.pos += 1

    # add the current snippet, if any
    self.add_current_snippet(start, end)
    
    return "".join(self.snippets)

  def line_before(self):
    i = self.pos
    while i > 0 and self.markup[i] != '\n':
      i -= 1
    return self.markup[i:self.pos]

  def add_current_snippet(self, start, end):
    snippet = self.markup[start:end]
    prev_snippet = None if len(self.snippets) == 0 else self.snippets[-1]
    
    # The second "and" clause below avoids multiple newlines. This is for
    # readability
    if snippet != "" and (snippet != "\n" or prev_snippet != "\n"):
      self.snippets.append(snippet)

    
  def snippet(self):
    return self.markup[self.pos:self.pos+50]

  def do_link(self):
    m = re.match(r"^\s*\[+((\w|\s|#)*?\|)?(.*?)\]+", self.markup[self.pos:])
    if m == None:
      # not a link, just one or more open brackets
      self.snippets.append(self.markup[self.pos])
      self.pos += 1
      return

    # language link?
    name = m.group(3)
    if ':' not in name:
      self.snippets.append(name)      
    self.pos += m.end()

  def do_format(self):
    m = re.match(r"('+)(.*?)\1", self.markup[self.pos:])
    if m == None:
      raise Exception("Invalid formatted text: '%s'..." % self.snippet())

    mc = MarkupToTextConverter()
    self.snippets.append(mc.convert(m.group(2)))
    self.pos += m.end()

  def do_tag(self):
    self.do_nested('tag', '<', '>')

  def do_meta(self):
    self.do_nested('meta', '{', '}')


  def do_nested(self, name, open_char, close_char):
    if self.markup[self.pos] != open_char:
      raise Exception("Invalid %s markup. Expected '{' at '%s...'" % (name, self.snippet()))

    count = 1
    while count > 0 and self.pos+1 < len(self.markup):
      self.pos += 1
      ch = self.markup[self.pos]
      if ch == open_char:
        count += 1
      elif ch == close_char:
        count -= 1

    self.pos += 1

  def do_heading(self):
    m = re.match(r"^\s*(=+)\s*(.+?)\s*\1", self.markup[self.pos:], flags=re.MULTILINE)
    if m == None:
      raise Exception("Invalid header at '%s'..." % self.snippet())
    
    title = m.group(2)
    self.snippets.append(title)
    self.pos += m.end()

  def do_nowiki(self):
    m = re.match("<nowiki>(.*?)</nowiki>", flags=re.MULTILINE)
    if m == None:
      raise Exception("Invalid 'nowiki' tag at '%s'..." % self.snippet())

    # ignore, as it may contain unsanitized markup
    self.pos += m.end()
    
    

class ProgressTracker:
  """Keeps track and displays progress of tasks"""
  
  def __init__(self, min_value, max_value, units):
    self.min_value = min_value
    self.max_value = max_value
    self.range     = max_value - min_value
    self.units     = units
    self.progress  = 0        # current normalized progress 0-1
    self.last_messsage = ""   # last printed message

  def clear_output(self):
    for ch in self.last_messsage:
      sys.stdout.write("\b")

  def update_output(self, message):
    self.clear_output()
    sys.stdout.write(message)
    sys.stdout.flush()
    self.last_messsage = message
    
  def update_progress(self, absolute_progress):
    self.progress = (float(absolute_progress) - self.min_value) / self.range
    percentage = 100.0*self.progress

    abs_string = "%.2f%s/%.2f%s" % (absolute_progress, self.units, self.max_value, self.units)
    pb_string = self.get_progressbar_str(self.progress)
    self.update_output("%-40s %s %.2f%%" % (abs_string, pb_string, percentage))

  def get_progressbar_str(self, progress):
    pb = "["
    for x in numpy.linspace(0,1,21):
      if x == 0: continue
      
      if progress >= x:
        pb += "="
      elif progress < x and progress > x - 0.025:
        pb += "-"
      else:
        pb += " "
    pb += "]"
    return pb
    
  

class Article:
  """ Stores the contents of a Wikipedia article """
  def __init__(self, title, markup, is_redirect):
    self.title = title
    self.markup = markup
    self.is_redirect = is_redirect
  

class WikiParser:
  """Parses the Wikipedia XML and extracts the relevant data,
     such as sentences and vocabulary"""

  
  def __init__(self, input_file_path, callback,
               ignore_redirects=True):
    self.callback = callback
    self.input_file_path = input_file_path
    self.input_file  = open(self.input_file_path, 'r')
    self.ignore_redirects = ignore_redirects
    self.buffer_size = 1024*1024 # 1MB
    self.input_size  = os.path.getsize(self.input_file_path)
    self.tracker = ProgressTracker(0, float(self.input_size) / (1024*1024), "MB")

    # Articles whose titles start with "<type>:" will be ignored.
    self.ignoredArticleTypes = ["wikipedia", "category", "template"]

    
    # setup the SAX XML parser and its callbacks
    self.xml_parser = sax.ParserCreate()
    self.xml_parser.StartElementHandler  = lambda name, attrs: self.xml_start_element(name, attrs)
    self.xml_parser.EndElementHandler    = lambda name:        self.xml_end_element(name)
    self.xml_parser.CharacterDataHandler = lambda data:        self.xml_char_data(data)

    # parser state
    self.article = None         # name of the current article
    self.section = None         # name of the current section
    self.word    = None         # current word
    self.enclosing_tags = []    # all enclosing tags (most recent first)
    self.text    = []           # contents of the current text element, in the order
                                # they come from the SAX parser
                                # (note: this is faster than concatenating on the fly)
    self.article = None         # article currently being processed

    
  def process(self):
    """Processes the file in its entirety"""

    self.tracker.update_progress(0)
    mbytes_read = 0.0
    while True:
      buf = self.input_file.read(self.buffer_size)
      if buf == "":
        break

      self.xml_parser.Parse(buf)
      
      mbytes_read += float(len(buf)) / (1024*1024)
      self.tracker.update_progress(mbytes_read)

  def xml_char_data(self, data):
    self.text.append(data)
    pass

  def xml_start_element(self, name, attrs):
    name = name.lower()
    self.enclosing_tags = [name] + self.enclosing_tags
    self.text = []

    if name == "page":
      self.article = Article(None, None, False)

  def xml_end_element(self, name):
    name = name.lower()
    contents = "".join(self.text)

    # dispatch based on the type of the node
    if name == "title":
      self.article.title = contents
    elif name == "redirect":
      self.article.is_redirect = True
    elif name == "text":
      self.article.markup = contents
    elif name == "page":
      if not (self.ignore_redirects and self.article.is_redirect):
        pass

      self.new_article(self.article)
      self.article = None

    # clean up state associated with the node    
    if len(self.enclosing_tags) > 0 and name == self.enclosing_tags[0]:
      self.enclosing_tags = self.enclosing_tags[1:]
    else:
      err("Mismatched closing tag: " + name)
    self.text = []

  def new_article(self, article):
    if ':' in  article.title:
      articleType = article.title.split(':')[0].lower()
      if articleType in self.ignoredArticleTypes:
        return

    self.callback(article)

  def get_enclosing_tag(self):
    return None if len(self.enclosing_tags) == 0 else self.enclosing_tags[0]
    
  def close(self):
    """Releases all resources associated with this class"""
    self.input_file.close()
