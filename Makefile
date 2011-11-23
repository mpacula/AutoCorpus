DIRS = src/wikipedia src/ngrams src/common
BIN = bin

all: wikipedia ngrams

common:
	cd src/common; $(MAKE) $(MFLAGS)

wikipedia: force_look common
	cd src/wikipedia; $(MAKE) $(MFLAGS)

ngrams: force_look common
	cd src/ngrams; $(MAKE) $(MFLAGS)

test: force_look
	cd src/tests; PATH="../../bin/:$$PATH"; ./driver.py tests.txt

doc:
	scripts/generate-doc.sh

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

force_look:
	true

install: all
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/share/pyshared/autocorpus
	for binary in `find $(BIN) -perm /u+x`; do cp -v $$binary $(DESTDIR)"/usr/bin"; done;
	mkdir -p $(DESTDIR)"/usr/share/pyshared/autocorpus"
	cp src/wikipedia/articles.py src/wikipedia/wiki.py $(DESTDIR)"/usr/share/pyshared/autocorpus"
	ln -sf "/usr/share/pyshared/autocorpus/articles.py" $(DESTDIR)"/usr/bin/wiki-articles"
	for sec in `seq 1 1 7`; do \
       if [ `ls man | grep "\.$$sec.gz" | wc -l )` -gt 0 ]; then \
	        mkdir -p $(DESTDIR)"/usr/share/man/man$$sec"; cp -v man/*.$$sec.gz $(DESTDIR)"/usr/share/man/man$$sec"; \
            fi; \
    done;
