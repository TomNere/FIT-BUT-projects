#include <iostream>
#include <string>
#include <unistd.h>
#include <regex>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "ipk.h"

using namespace std;

void listen(string port) {

    int client_socket;
    struct sockaddr_in address;
    struct hostent* server;

    bzero((char *) &address, sizeof(address));
    address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&address.sin_addr.s_addr, server->h_length);
    address.sin_port = htonl(stoul(info.port, NULL, 10));
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Create socket
    if(client_socket = socket(AF_INET, SOCK_STREAM, 0) <= 0) {
        ERR_RET("Socket error!")
    }

    memset(&sa,0,sizeof(sa));
sa.sin6_family = AF_INET6;
sa.sin6_addr = in6addr_any;
sa.sin6_port = htons(port_number);
if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0)
{
perror("ERROR: bind");
exit(EXIT_FAILURE);
}
}

int main(int argc, char const *argv[]) {
    
    int option;
    string port = "";

    while((option = getopt(argc, (char**) argv, "p:")) != -1) {
        switch(option) {
            case 'p':
                if (port.compare("")) {
                    ERR_RET("Invalid parameters!");
                }
                port = optarg;
                break;
            default:
                ERR_RET("Invalid parameters!");
        }
    }

    if (!port.compare("")) {
        ERR_RET("Missing parameters!");
    }

    // Port check
    if (port.find_first_not_of("0123456789") == string::npos) {
        if (stoi(port, NULL, 10) > 65535) {
            ERR_RET("Invalid port!");
        }
    }
    else {
        ERR_RET("Invalid port!");
    }

    listen(port);

    return 0;
}