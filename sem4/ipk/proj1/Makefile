all: client server

client: 
	g++ ipk-client.cpp ipk.h -o ipk-client
	
server:
	g++ ipk-server.cpp ipk.h -o ipk-server

clean:
	rm -f ipk-client ipk-server
