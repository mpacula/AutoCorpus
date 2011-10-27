DIRS = wikipedia ngrams

all: wikipedia ngrams

wikipedia: force_look
	cd wikipedia; $(MAKE) $(MFLAGS)

ngrams: force_look
	cd ngrams; $(MAKE) $(MFLAGS)

clean:
	-for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

force_look:
	true
