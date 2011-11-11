DIRS = wikipedia ngrams
BIN = ../bin

all: wikipedia ngrams

wikipedia: force_look
	cd wikipedia; $(MAKE) $(MFLAGS)

ngrams: force_look
	cd ngrams; $(MAKE) $(MFLAGS)

doc:
	../scripts/generate-doc.sh

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

force_look:
	true

install: all
	for binary in `find $(BIN) -perm /u+x`; do cp -v $$binary /usr/bin; done;
	mkdir -p /usr/share/pyshared/autocorpus
	cp ../src/wikipedia/articles.py ../src/wikipedia/wiki.py /usr/share/pyshared/autocorpus
	ln -sf /usr/share/pyshared/autocorpus/articles.py /usr/bin/wiki-articles
	mkdir -p /usr/share/man/man1
	for sec in `seq 1 1 7`; do cp -v ../man/*.$$sec.gz /usr/share/man/man$$sec; done;
