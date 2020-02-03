all: clean ipk-mtrip

ipk-mtrip: 
	g++ -static-libstdc++ -std=c++11 -lpthread ipk-mtrip.cpp -o ipk-mtrip

clean:
	rm -f ipk-mtrip
