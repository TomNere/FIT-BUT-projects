#include <iostream>     // I/O operations
#include <unistd.h>     // getopt etc.   
#include <string.h>     // bzero, bcopy
#include <arpa/inet.h>  // inet_pton
#include <sys/socket.h> // Sockets
#include <netdb.h>      // gethostbyname...
#include <netinet/in.h> // sockaddr_in...

using namespace std;

// Error messages...
const string client_help = "Invalid parameters!\n Usage: ./ipk-client -h host -p port [-n|-f|-l] login\n"
                           "       ./ipk-client -h host -p port -l\n";
#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

// Class representing all information about request
class InputInfo {
    public: 
    string host, port, login, req_message;

    /* This int signalize which argument was selected
    * n - 1
    * f - 2
    * l - 3
    */
    int req_type;

    // Constructor
    InputInfo() {
        host = req_message = port = login = "";
        req_type = 0;
    }

    // Validate arguments
    void validate() {
        if (!host.compare("") || !port.compare("")) {           // Check for host and port
            ERR_RET("Missing parameters!");
        }

        if (isdigit(host[0])) {         // If digit, check for valid IPv4 server_address
            // Just for IPv4 check
            in_addr tmp_ip;

            if (inet_pton(AF_INET, host.c_str(), &tmp_ip) != 1) {
                ERR_RET("Invalid IP server_address!");
            }
        }
       
        if (port.find_first_not_of("0123456789") == string::npos) {      // Check for valid port
            if (stoi(port, NULL, 10) > 65535) {
                ERR_RET("Invalid port!");
            }
        }
        else {
            ERR_RET("Invalid port!");
        }

        if (req_type == 0) {                // Missing [n|f|l]
            ERR_RET("Missing parameters!");
        }

        if (req_type != 3) {                // Login is required for -n and -f
            if(!login.compare("")) {
                ERR_RET("Missing parameters!");       
            }
        }

        if (login.length() > (1022 - 1)) {  // Too long login to send in 1 buffer
            ERR_RET("Unsupported length of login!"); 
        }

        req_message = to_string(req_type) + "&" + login;    // Message sent to server
    }
};

// Send request and print message from server
void request(InputInfo info) {
    // Create socket
    int client_socket;
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        ERR_RET("Socket error!")
    }
    // Set timeout
    // (https://stackoverflow.com/questions/30395258/setting-timeout-to-recv-function)
    struct timeval tv;
    tv.tv_sec = 15;        // 15 Secs Timeout
    tv.tv_usec = 0;        // Not init'ing this can cause strange errors
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
    // Get network host entry
    struct hostent* server;
    if ((server = gethostbyname(info.host.c_str())) == NULL) {
        ERR_RET("Host not found!");
    }
    
    // Create necessary structures
    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(stoul(info.port, NULL, 10));        // Port is string, need conversion

    // Connect
    if ((connect(client_socket, (const struct sockaddr *)&server_address, sizeof(server_address))) != 0) {
        ERR_RET("Connection error!");
    }

    // Send 1 request message
    int bytestx = send(client_socket, info.req_message.c_str(), info.req_message.length() + 1, 0);      // + 1 for '\0' character 
    if (bytestx < 0) {
        ERR_RET("Sending error!");
    }

    // Variables for data receiving
    // Notice \0 character at the end of C-style string
    const int BUFSIZE = 1024;
    int bytesrx;
    char buff[BUFSIZE];
    string received_msg = "";
    // Receive data in loop
    while ((bytesrx = recv(client_socket, buff, BUFSIZE, 0)) > 0) {
        cout << buff;
        // Send OK message
        int bytestx = send(client_socket, "OK", 3, 0);      // + 1 for '\0' character 
        if (bytestx < 0) {
            ERR_RET("Sending error!");
        }
        if (bytesrx < BUFSIZE) {
            break;
        }
    }
    if (bytesrx < 0) {
        ERR_RET("Receiving error!");
    }
    close(client_socket);           // Close socket
}

/* Main function 
* check parameters and call functions validate() and request()
*/
int main(int argc, char const *argv[])
{
    // Create object
    InputInfo info;

    int option;
    opterr = 0;         // Don't print error message if error occur

    while((option = getopt(argc, (char**) argv, ":h:p:n:f:l:")) != -1) {
        switch(option) {
            case 'h':
                if (info.host.compare("")) {
                    ERR_RET("Invalid parameters!");
                }
                info.host = string(optarg);
                break;
            case 'p':
                if (info.port.compare("")) {
                    ERR_RET("Invalid parameters!");
                }
                info.port = string(optarg);
                break;
            case 'n':
                if (info.req_type != 0) {
                    ERR_RET("Invalid parameters!");
                }
                info.req_type = 1;
                info.login = string(optarg);
                break;
            case 'f':
                if (info.req_type != 0) {
                    ERR_RET("Invalid parameters!");
                }
                info.req_type = 2;
                info.login = string(optarg);
                break;
            case 'l':
                if (info.req_type != 0) {
                    ERR_RET("Invalid parameters!");
                }
                info.req_type = 3;
                info.login = string(optarg);
                break;
            case ':':
                if (optopt == 'l') {
                    info.req_type = 3;
                }
                else {
                    ERR_RET("Invalid parameters!");
                }
                break;
            default:
                ERR_RET("Invalid parameters!");
        }
    }

    info.validate();
    request(info);

    return 0;
}
