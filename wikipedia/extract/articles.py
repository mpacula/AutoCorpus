#!/usr/bin/env python

import sys
import os
import numpy
import re
import codecs
import xml.parsers.expat as sax
from optparse import OptionParser
from wiki import *
    
def fprint(x):
  print x

def save_article(article, directory):
  filename = re.sub("(\s+)|/", "_", article.title.lower()) + ".txt"
  f = codecs.open(os.path.join(directory, filename), encoding='utf-8', mode='w')
  f.write(article.markup)
  f.write("\n")
  f.close()

def print_article(article):
  print unicode(article.markup).encode("utf-8")
  print "\n\n\f"

if __name__ == "__main__":
  try:
    parser = OptionParser(usage="usage: %s [-d output-directory] <stdin>" % sys.argv[0])
    parser.add_option("-d",
                      action="store", type="string", dest="directory",
                      help="directory where to store the articles")
    (options, args) = parser.parse_args()

    if len(args) > 0:
      print parser.usage
      exit(1)

    if options.directory != None:
      do_article = lambda a: save_article(a, options.directory)
    else:
      do_article = lambda a: print_article(a)

    parser = WikiParser(do_article)
    parser.process()
    parser.close()
  except KeyboardInterrupt:
    print "\n\nCancelled. Partial results may have been generated."
    exit(1)
