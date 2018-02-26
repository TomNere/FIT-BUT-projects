#include <iostream>
#include <cstring>
#include <unistd.h>
#include <regex>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "ipk-client.h"

using namespace std;

class InputInfo {
    public: 
    string host, port, login;

    /* This int signalize which argument was selected
    * n - 1
    * f - 2
    * l - 3
    */
    int req_type;
    int host_type;

    bool h_flag, p_flag;

    void initialize() {
        host = port = login = "";
        req_type = 0;
        h_flag = p_flag = false;
    }

    bool emptyComp(string str) {
        if (str.compare("")) {
            return true;
        }
        else {
            return false;
        }
    }

    void validate() {
        if (!host.compare("") || !port.compare("")) {
            ERR_RET("Missing parameters!");
        }
        // Host check
        if (isdigit(host[0])) {
            // Just for IP check
            unsigned char buffer[sizeof(struct in6_addr)];

            if (inet_pton(AF_INET, host.c_str(), buffer) != 1) {
                ERR_RET("Invalid IP address!");
            }
            host_type = 1;
        }
        else {
            host_type = 2;
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
        if (req_type == 0) {
            ERR_RET("Missing parameters!");
        }

        // login is needed
        if (req_type != 3) {
            if(!login.compare("")) {
                ERR_RET("Missing parameters!");       
            }
        }
    }
};

// send request to the server
void request(InputInfo info) {
    int my_socket;
    struct sockaddr_in address;
    struct hostent* server;

    if ((server = gethostbyname(info.host.c_str())) == NULL) {
        ERR_RET("Host not found!");
    }
    bzero((char *) &address, sizeof(address));
    address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&address.sin_addr.s_addr, server->h_length);
    address.sin_port = htonl(stoul(info.port, NULL, 10));

    // Create socket
    if(my_socket = socket(AF_INET, SOCK_STREAM, 0) <= 0) {
        ERR_RET("Socket error!")
    }
    // Connect
    if (connect(my_socket, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        ERR_RET("Connection error!");
    }

}

int main(int argc, char const *argv[])
{
    InputInfo info;
    info.initialize();

    int option;

    while((option = getopt(argc, (char**) argv, "h:p:n:f:l::")) != -1) {
        switch(option) {
            case 'h':
                if (info.emptyComp(info.host)) {
                    ERR_RET("Invalid parameters!");
                }
                info.host = optarg;
                break;
            case 'p':
                if (info.emptyComp(info.port)) {
                    ERR_RET("Invalid parameters!");
                }
                info.port = optarg;
                break;
            case 'n':
                if (info.req_type != 0) {
                    ERR_RET("Invalid parameters!");
                }
                info.req_type = 1;
                info.login = optarg;
                break;
            case 'f':
                if (info.req_type != 0) {
                    ERR_RET("Invalid parameters!");
                }
                info.req_type = 2;
                info.login = optarg;
                break;
            case 'l':
                if (info.req_type != 0) {
                    ERR_RET("Invalid parameters!");
                }
                info.req_type = 3;
                if (optarg != NULL) {
                    info.login = optarg;    
                }
                break;
            default:
                ERR_RET("Invalid parameters!");
        }
    }

    info.validate();

    return 0;
}
