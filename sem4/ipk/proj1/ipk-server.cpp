#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <regex>
#include <ctype.h>
#include <cerrno>           // For debugging
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "ipk.h"

using namespace std;

string parseReq(string req_message) {
    ifstream file;
    file.open("/etc/passwd");
    string line;

    switch(req_message[0]) {
        case '1':
            while(getline(file, line)) {
                if (line.find(req_message.substr(2)) != string::npos) {
                    int index;
                    for(int i = 1; i <= 4; i++) {
                        if(line.find(":") == string::npos) {
                            break;
                        }
                        line.erase(line.find(":"));
                    }
                    file.close();
                    return (line.substr(0, line.find(":")) + "\0");
                }
            }
            break;
        case '2':
            break;
        case '3':
            break;
        default:
            ERR_RET("Invalid request from client!");
    }
}

void listen(string port) {

    int server_socket;

    // Create and set socket
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        ERR_RET("Socket error!");
    }
    //int optval = 1;
    //setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    // Bind
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(stoul(port, NULL, 10));

    int rc;
    if ((rc = bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address))) < 0) {
        cout << strerror(errno) << endl;
        ERR_RET("Binding error!");
    }

    if ((listen(server_socket, 1)) < 0) {
        ERR_RET("Listen error!");
    }
    size_t len = sizeof(server_address);
    while(true) {
        int comm_socket = accept(server_socket, (struct sockaddr*)&server_address, (socklen_t*)&len);
        
        if (comm_socket > 0) {
            char buff[1024];
            int res = 0;

            while(true) {
                if ((res = recv(comm_socket, buff, 1024, 0)) < 0) {
                    ERR_RET("Error receiving from client!");
                }
                const char* sent_buff = parseReq(string(buff)).c_str();
                //cout << string(sent_buff) << endl;
                send(comm_socket, sent_buff, strlen(sent_buff), 0);
                if (res < 1024) {
                    break;
                }
                
            }
        }
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