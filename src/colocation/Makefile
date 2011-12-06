CC=ghc
BIN = ../../bin

all: $(BIN)/colocations

$(BIN)/colocations: Colocations.hs
	$(CC) -O3 -threaded -rtsopts Colocations.hs -o $(BIN)/colocations

prof: Colocations.hs
	$(CC) -prof -auto-all -rtsopts Colocations.hs -o $(BIN)/colocations

clean:
	rm *.hi $(BIN)/colocations
