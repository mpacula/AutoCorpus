CC = g++
LIBS = -lpcre
CFLAGS = -Wall -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out Sentences.o Textify.o, $(ALL_OBJFILES))
BIN = ../../bin

all: TAGS $(BIN)/wiki-textify $(BIN)/sentences $(BIN)/wiki-articles

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

$(BIN)/wiki-textify: $(OBJFILES) Textify.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) Textify.o -lpcre -o $(BIN)/wiki-textify

$(BIN)/sentences: $(OBJFILES) Sentences.o
	${CC} $(CFLAGS) $(OBJFILES) Sentences.o -lpcre -o $(BIN)/sentences

$(BIN)/wiki-articles: articles.py
	ln -fs ../src/wikipedia/articles.py $(BIN)/wiki-articles

%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o $(BIN)/wiki-textify $(BIN)/sentences $(BIN)/wiki-articles TAGS
