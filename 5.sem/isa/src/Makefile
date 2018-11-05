all: clean dns-export

dns-export: 
	g++ -g -static-libstdc++ -std=c++11 dns-export.cpp -o dns-export -lpcap

clean:
	rm -f dns-export
