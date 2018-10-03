all: clean dns-export

dns-export: 
	g++ -static-libstdc++ -std=c++11 -lpcap dns-export.cpp  dns-export.h -o dns-export

clean:
	rm -f dns-export
