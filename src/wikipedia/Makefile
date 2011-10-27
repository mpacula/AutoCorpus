CC = g++
LIBS = -lpcre
CFLAGS = -Wall -O3
COMPILE = $(CC) $(CFLAGS) -c
ALL_OBJFILES = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJFILES := $(filter-out Sentences.o Textify.o, $(ALL_OBJFILES))

all: tags ../bin/textify ../bin/sentences

tags: $(wildcard *.cpp)
	etags $(wildcard *.cpp)

../bin/textify: $(OBJFILES) Textify.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) Textify.o -lpcre -o ../bin/textify

../bin/sentences: $(OBJFILES) Sentences.o
	${CC} $(CFLAGS) -lpcre $(OBJFILES) Sentences.o -lpcre -o ../bin/sentences

%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o ../bin/textify ../bin/sentences TAGS
