CC = g++
LIBS = -lpcre
CFLAGS = -Wall -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out Extract.o Textify.o, $(ALL_OBJFILES))

all: bin/textify bin/extract

bin/textify: $(OBJFILES) Textify.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) Textify.o -lpcre -o bin/textify

bin/extract: $(OBJFILES) Extract.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) Extract.o -lpcre -o bin/extract

%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o bin/*
