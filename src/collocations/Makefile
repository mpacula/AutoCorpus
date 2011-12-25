CC=g++
BIN = ../../bin
OPTIONS =  -Wall -Iboost -I../common -std=c++0x -lpcre
COMMON_OBJ = ../common/utilities.o ../common/merge.o

all: $(BIN)/collocations

prof: $(BIN)/collocations-prof

$(BIN)/collocations: Collocations.cpp
	$(CC) $(COMMON_OBJ) Collocations.cpp $(OPTIONS) -O3 -o $(BIN)/collocations

$(BIN)/collocations-prof:
	$(CC) $(COMMON_OBJ) Collocations.cpp $(OPTIONS) -g -o $(BIN)/collocations-prof

clean:
	rm -f *$(BIN)/cololcations
