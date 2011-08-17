#!/usr/bin/env python

# Processes and transforms Wikipedia article dumps into corpora
# suitable for Natural Language Processing.
#
# (c) Maciej Pacula 2011
#

import sys
import os
import numpy
import re
import xml.parsers.expat as sax
from optparse import OptionParser

def err(msg):
  """ prints a message to stderr, terminating it with a newline """
  sys.stderr.write(msg + "\n")


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
    
  

class ParserCallback:
  """ Handles parser events such as encountering articles, sections etc """
  def __init__(self, article):
    self.article = article


def fprint(x):
  """ makes print available as a first class function """
  print x
  
printing_callback = ParserCallback(fprint)
    
class Article:
  """ Stores the contents of a Wikipedia article """
  def __init__(self, title, text, is_redirect):
    self.title = title
    self.text = text
    self.is_redirect = is_redirect
  

class WikiParser:
  """Parses the Wikipedia XML and extracts the relevant data,
     such as sentences and vocabulary"""

  
  def __init__(self, input_file_path, output_file_path, callback,
               ignore_redirects=True):
    self.callback = callback
    self.input_file_path = input_file_path
    self.output_file_path = output_file_path
    self.input_file  = open(self.input_file_path, 'r')
    self.output_file = open(self.output_file_path, 'w')
    self.ignore_redirects = ignore_redirects
    self.buffer_size = 10*1024*1024 # 10MB
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
    self.text = [];

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
      self.article.text = contents
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
    self.text = [];

  def new_article(self, article):
    if ':' in  article.title:
      articleType = article.title.split(':')[0].lower()
      if articleType in self.ignoredArticleTypes:
        return

    sanitized = article.text
    # text-ify named article hyperlinks e.g. [United States|American]
    sanitized = re.sub(r"\[\[(.+?)\|(.*?)\]\]", r"\2", sanitized)

    # textify unnamed article hyperlinks e.g. [United States]
    sanitized = re.sub(r"\[\[(.+?)\]\]", r"\1", sanitized)

    # remove metadata
    sanitized = re.sub(r"\{\{.+?\}\}", r"", sanitized, flags=re.DOTALL)

    # remove links to pages in other languages
    sanitized = re.sub(r"^[a-zA-Z-]+\:.+?$", "", sanitized, flags=re.MULTILINE)

    # remove hard paragraph breaks
    sanitized = re.sub(r"<\s*\w+\s*/?\s*>", "", sanitized)
    print sanitized

  def get_enclosing_tag(self):
    return None if len(self.enclosing_tags) == 0 else self.enclosing_tags[0]
    
  def close(self):
    """Releases all resources associated with this class"""
    self.input_file.close()
    self.output_file.close()

    

if __name__ == "__main__":
  try:
    parser = OptionParser(usage="usage: sentences.py [options] input-file")
    parser.add_option("--output",
                      action="store", type="string", dest="output",
                      help="file where to store the output sentences")
    (options, args) = parser.parse_args()

    if len(args) == 0:
      err("No input Wikipedia dump file specified.")
      exit(1)
    elif len(args) > 1:
      err("Too many input arguments: " + str(args))
      exit(1)

    if options.output == None:
      err("No output file specified.")
      exit(1)


    input_file_path = args[0]
    print "Extracting sentences..."
    parser = WikiParser(input_file_path, options.output, printing_callback)
    parser.process()

    print "\nCleaning up..."
    parser.close()
    print "Done. Have fun!"
  except KeyboardInterrupt:
    print "\n\nCancelled. Partial results may have been generated."
    exit(1)
