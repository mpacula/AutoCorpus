all: colocations-hs colocations-cpp

colocations-hs: Colocations.hs
	ghc -O3 -Wall Colocations.hs -o colocations-hs

colocations-cpp: Colocations.cpp
	g++ -O3 -Wall -std=c++0x -Iboost Colocations.cpp -o colocations-cpp

clean:
	rm -rf colocations-hs colocations-cpp *.hi *.o
