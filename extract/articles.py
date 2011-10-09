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
    f.close()

if __name__ == "__main__":
  try:
    parser = OptionParser(usage="usage: articles.py -d output-directory input-file")
    parser.add_option("-d",
                      action="store", type="string", dest="directory",
                      help="directory where to store the articles")
    (options, args) = parser.parse_args()

    if len(args) == 0:
      print parser.usage
      exit(1)
    elif len(args) > 1:
      print parser.usage
      exit(1)

    if options.directory == None:
      print parser.usage
      exit(1)

    input_file_path = args[0]
    print "Extracting articles..."
    parser = WikiParser(input_file_path, lambda a: save_article(a, options.directory))
    parser.process()

    print "\nCleaning up..."
    parser.close()
    print "Done. Have fun!"
  except KeyboardInterrupt:
    print "\n\nCancelled. Partial results may have been generated."
    exit(1)
