CC = g++
LIBS = 
CFLAGS = -Wall -std=c++0x -O3 -lpcre
COMPILE = $(CC) $(CFLAGS)
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(ALL_OBJFILES)
BIN = ../../bin

all: TAGS $(OBJFILES) $(BIN)/merge-counts

$(BIN)/merge-counts: MergeCounts.cpp merge.o utilities.o
	$(COMPILE) merge.o utilities.o -lpcre MergeCounts.cpp -o $(BIN)/merge-counts

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

%.o: %.cpp Makefile
	$(COMPILE) -c -o $@ $<

clean:
	rm -f *.o TAGS
