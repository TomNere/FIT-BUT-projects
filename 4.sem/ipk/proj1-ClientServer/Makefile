all: client server

client: 
	g++ -static-libstdc++ ipk-client.cpp -o ipk-client
	
server:
	g++ -static-libstdc++ ipk-server.cpp -o ipk-server

clean:
	rm -f ipk-client ipk-server
