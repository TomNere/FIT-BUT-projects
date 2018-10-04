all: clean dns-export

dns-export: 
	g++ -static-libstdc++ -std=c++11 dns-export.cpp  dns-export.h -o dns-export -lpcap

clean:
	rm -f dns-export
