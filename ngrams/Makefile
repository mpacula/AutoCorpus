CC = g++
LIBS =
CFLAGS = -Wall -std=c++0x -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out NGramCounter.o, $(ALL_OBJFILES))

all: bin/ngrams

bin/ngrams: $(OBJFILES) NGramCounter.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) NGramCounter.o -o bin/ngrams

%.o: %.cpp Makefile
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o bin/*
