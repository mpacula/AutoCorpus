CC = g++
LIBS = 
CFLAGS = -Wall -std=c++0x -O3
COMPILE = $(CC) $(CFLAGS)
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(ALL_OBJFILES)
LIBS= -lpcre -lrt
BIN = ../../bin

all: TAGS $(OBJFILES) $(BIN)/merge-counts $(BIN)/truncate

$(BIN)/merge-counts: MergeCounts.cpp merge.o utilities.o
	$(COMPILE) merge.o utilities.o MergeCounts.cpp $(LIBS) -o $(BIN)/merge-counts

$(BIN)/truncate: truncate.cpp
	$(COMPILE) truncate.cpp -o $(BIN)/truncate

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

%.o: %.cpp Makefile
	$(COMPILE) -c -o $@ $<

clean:
	rm -f *.o TAGS
