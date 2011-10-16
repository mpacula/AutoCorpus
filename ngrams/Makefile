CC = g++
LIBS = 
CFLAGS = -Wall -std=c++0x -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out NGramCounter.o Tokenizer.o Filter.o, $(ALL_OBJFILES))

all: bin/ngrams bin/tokenize bin/filter

bin/ngrams: $(OBJFILES) NGramCounter.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) NGramCounter.o -o bin/ngrams

bin/tokenize: $(OBJFILES) Tokenizer.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) Tokenizer.o -o bin/tokenize

bin/filter: $(OBJFILES) Filter.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) Filter.o -o bin/filter

%.o: %.cpp Makefile
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o bin/*
