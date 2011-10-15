CC = g++
LIBS = 
CFLAGS = -Wall -std=c++0x -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out NGramCounter.o Tokenizer.o, $(ALL_OBJFILES))

all: bin/ngrams bin/tokenize

bin/ngrams: $(OBJFILES) NGramCounter.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) NGramCounter.o -o bin/ngrams

bin/tokenize: $(OBJFILES) Tokenizer.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) Tokenizer.o -o bin/tokenize

%.o: %.cpp Makefile
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o bin/*
