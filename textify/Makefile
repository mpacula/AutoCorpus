CC = g++
LIBS = -lpcre
CFLAGS = -Wall -O3
COMPILE = $(CC) $(CFLAGS) -c
OBJFILES := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

bin/textify: $(OBJFILES)
	${CC} -lpcre $(OBJFILES) -lpcre -o bin/textify

%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o bin/*
