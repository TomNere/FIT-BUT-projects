all: clean dns-export

dns-export: 
	g++ -static-libstdc++ -std=c++11 -pthread dns-export.cpp -o dns-export -lpcap

clean:
	rm -f dns-export
