CPP=g++
LIBS="-lpcre"

textify: textify.cpp
	${CPP} textify.cpp $(LIBS) -O3 -o textify

clean:
	rm -f textify.o textify
