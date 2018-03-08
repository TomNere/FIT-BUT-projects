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
#include <netinet/in.h>
#include <netdb.h>
#include "ipk.h"

using namespace std;

string parseReq(string req_message) {
    ifstream file;
    file.open("/etc/passwd");
    string line, lines;

    switch(req_message[0]) {
        case '1':
            while(getline(file, line)) {
                if (line.substr(0, line.find(":")).compare(req_message.substr(2)) == 0) {
                    int index;
                    for(int i = 1; i <= 4; i++) {
                        if(line.find(":") == string::npos) {
                            break;
                        }
                        line.erase(0, line.find(":") + 1);
                    }
                    file.close();
                    return (line.substr(0, line.find(":")));
                }
            }
            break;
        case '2':
            while(getline(file, line)) {
                if (line.substr(0, line.find(":")).compare(req_message.substr(2)) == 0) {
                    int index;
                    for(int i = 1; i <= 5; i++) {
                        if(line.find(":") == string::npos) {
                            break;
                        }
                        line.erase(0, line.find(":") + 1);
                    }
                    file.close();
                    return (line.substr(0, line.find(":")));
                }
            }
            break;
        case '3':
            lines = "";
            while(getline(file, line)) {
                if ((req_message.substr(2)).compare("-") == 0) {
                    lines = lines + line.substr(0, line.find(":")) + "\n";   
                }
                else if (line.substr(0, req_message.substr(2).length()).compare(req_message.substr(2)) == 0) {
                    lines = lines + line.substr(0, line.find(":")) + "\n";
                }
            }
            if (lines.length() > 0) {
                lines.resize(lines.size() - 1);    
            }
            return lines;
            break;
        default:
            ERR_RET("Invalid request from client!");
    }
    return "";
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
        cerr << strerror(errno) << endl;
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
            string received_request = "";

            while(true) {
                if ((res = recv(comm_socket, buff, 1024, 0)) < 0) {
                    ERR_RET("Error receiving from client!");
                }
                received_request = received_request + string(buff);
                if (res < 1024) {
                    break;
                }
            }
            string data_to_send = parseReq(received_request);
            size_t size_to_send = data_to_send.length();
            string tmp = "";

            if (size_to_send == 0) {
                send(comm_socket, "\0", 1, 0);
            }

            while(size_to_send) {
                // Notice \0 character at the end of C-style string
                if (size_to_send > 1024) {
                    tmp = data_to_send.substr(0, 1023);
                    data_to_send.erase(0, 1023);
                    size_to_send = size_to_send - 1023;
                }
                else {
                    tmp = data_to_send;
                    size_to_send = 0;
                }
                send(comm_socket, tmp.c_str(), tmp.length() + 1, 0);
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