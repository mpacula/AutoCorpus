CC = g++
LIBS = -lpcre
CFLAGS = -Wall -O3 -I "../common"
COMMON_OBJ = ../common/utilities.o ../common/PCREMatcher.o
COMPILE = $(CC) $(CFLAGS) -c 
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES = $(filter-out Sentences.o Textify.o, $(ALL_OBJFILES)) $(COMMON_OBJ)
BIN = ../../bin
LIB = ../../lib

all: TAGS $(BIN)/wiki-textify $(BIN)/sentences $(BIN)/wiki-articles

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

$(BIN)/wiki-textify: $(OBJFILES) Textify.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) Textify.o -lpcre -o $(BIN)/wiki-textify

$(BIN)/sentences: $(OBJFILES) Sentences.o
	${CC} $(CFLAGS) $(OBJFILES) Sentences.o -lpcre -o $(BIN)/sentences

$(BIN)/wiki-articles: articles.py wiki.py
	mkdir -p $(LIB)
	cp wiki.py articles.py $(LIB)/
	ln -fs ../lib/articles.py $(BIN)/wiki-articles

%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o $(BIN)/wiki-textify $(BIN)/sentences $(BIN)/wiki-articles TAGS
