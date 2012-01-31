CC = g++
LIBS = -lpcre -lrt -lboost_thread
COMMON_OBJ = ../common/merge.o ../common/PCREMatcher.o ../common/utilities.o
CFLAGS = -Wall -std=c++0x -O3 -I "../common"
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES = $(filter-out Collocations.o MutualInformation.o, $(ALL_OBJFILES)) $(COMMON_OBJ)
BIN = ../../bin

all: $(BIN)/collocations $(BIN)/mutual-information

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

$(BIN)/collocations: $(OBJFILES) Collocations.o
	${CC} $(CFLAGS) $(OBJFILES) Collocations.o $(LIBS) -o $(BIN)/collocations

$(BIN)/mutual-information: $(OBJFILES) MutualInformation.o
	${CC} $(CFLAGS) $(OBJFILES) MutualInformation.o $(LIBS) -o $(BIN)/mutual-information

%.o: %.cpp Makefile
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o TAGS $(BIN)/collocations $(BIN)/mututal-information
