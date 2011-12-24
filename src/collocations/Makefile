CC=g++
BIN = ../../bin
OPTIONS =  -Iboost -std=c++0x -lpcre
COMMON_OBJ = ../common/utilities.o ../common/merge.o

all: $(BIN)/collocations

$(BIN)/collocations: Collocations.cpp
	$(CC) $(COMMON_OBJ) Collocations.cpp $(OPTIONS) -O3 -o $(BIN)/collocations

clean:
	rm -f *$(BIN)/cololcations
