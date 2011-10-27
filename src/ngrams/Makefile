CC = g++
LIBS = 
CFLAGS = -Wall -std=c++0x -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out NGramCounter.o Tokenizer.o Filter.o, $(ALL_OBJFILES))
BIN = ../../bin

all: TAGS $(BIN)/ngrams $(BIN)/tokenize $(BIN)/ngrams-freq-filter

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

$(BIN)/ngrams: $(OBJFILES) NGramCounter.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) NGramCounter.o -o $(BIN)/ngrams

$(BIN)/tokenize: $(OBJFILES) Tokenizer.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) Tokenizer.o -o $(BIN)/tokenize

$(BIN)/ngrams-freq-filter: $(OBJFILES) Filter.o
	${CC} $(CFLAGS) $(LIBS) $(OBJFILES) Filter.o -o $(BIN)/ngrams-freq-filter

%.o: %.cpp Makefile
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o TAGS $(BIN)/ngrams $(BIN)/tokenize $(BIN)/ngrams-freq-filte
