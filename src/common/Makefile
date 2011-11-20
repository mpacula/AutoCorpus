CC = g++
LIBS = 
CFLAGS = -Wall -std=c++0x -O3 -lpcre
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(ALL_OBJFILES)
BIN = ../../bin

all: TAGS $(OBJFILES)

TAGS: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

%.o: %.cpp Makefile
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o TAGS
