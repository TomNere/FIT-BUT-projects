#include <fstream>      // FIle
#include <signal.h>     // SIGINT handling
#include <iostream>     // I-O operations
#include <string.h>     // memset, strcmp
#include <unistd.h>     // getopt...  
#include <sys/socket.h> // Sockets...
#include <netinet/in.h> // sockaddr_in...

using namespace std;
int welcome_socket;         // Global variable - possibility of closing socket in signalHandler

// Error messages...
const string server_help = "Invalid parameter!\n Usage: ./ipk-server -p port\n";
#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

// Close welcome socket and terminate
void signalHandler(int signum) {
    cout << "\nInterrupt signal (" << signum << ") received.\nTerminating.\n";
    close(welcome_socket);
    exit(signum);
}

// Get name(pos == 4) or home folder(pos == 5) from line
string getSubstr(string line, int pos) {
    int index;
    for(int i = 0; i < pos; i++) {                
        if(line.find(":") == string::npos) {    // Error - : wasn't found
            break;
        }
        line.erase(0, line.find(":") + 1);      // Remove part of string to first ':' 
    }
    return ((line.substr(0, line.find(":"))) + "\n");    // Return :substring:
}

// This function parse request message and create string to send
string parseReq(string req_message) {
    // Open file
    ifstream file;
    file.open("/etc/passwd");

    if (!file) {
        return "";
    }

    string line, lines;
    line = lines = "";

    if ((req_message.substr(0, req_message.find("&")).compare("<--xnerec00_protocol-->") != 0) || req_message.length() < string("<--xnerec00_protocol-->&1").length()) {
        return "Invalid request!\n";
    }

    string login = req_message.substr(req_message.find("&") + 3);

    switch(req_message[req_message.find("&") + 1]) {        // First character in req_message is type of request
        case '1':                   // -n parameter - name
            while(getline(file, line)) {
                if (line.substr(0, line.find(":")).compare(login) == 0) {   // If login was found
                    file.close();
                    return getSubstr(line, 4);    // Name is after 4. ':'
                }
            }
            break;
        case '2':                   // -f parameter - home folder
            while(getline(file, line)) {
                if (line.substr(0, line.find(":")).compare(login) == 0) {   // If login was found
                    file.close();
                    return getSubstr(line, 5);    // Home folder is after 5. ':'
                }
            }
            break;
        case '3':                   // -l parameter - login
            if (login.compare("") == 0) {            // If no login, create string of all logins
                while(getline(file, line)) {
                    lines = lines + line.substr(0, line.find(":")) + "\n";
                }
            }
            else {
                int length = login.length(); 
                while(getline(file, line)) {
                    if (line.substr(0, length).compare(login) == 0) {  // Find match with prefix
                        lines = lines + line.substr(0, line.find(":")) + "\n";
                    }
                }
            }
            file.close();
            return lines;
        default:
            return "";
    }
    return "";
}

// Listening on port and resolving requests
void listen(string port) {
    // Create welcome socket
    if((welcome_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        ERR_RET("Socket error!");
    }
    // Allow binding when port seems to be used(e.g. after restarting this app)
    // (https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr)
    int enable = 1;
    if (setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        ERR_RET("Error setting setsockopt(SO_REUSEADDR)!");
    }

    // Create necessary structures for Bind
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(stoul(port, NULL, 10));     // Port is string, need conversion
    int rc;

    // Bind
    if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0) {
        ERR_RET("Binding error!");
    }
    // Listen
    if ((listen(welcome_socket, 1)) < 0) {
        ERR_RET("Listening error!");
    }

    // Create necessary structures for Accept
    struct sockaddr_in sa_client;
    socklen_t sa_client_len = sizeof(sa_client);
    const int BUFSIZE = 1024;

    while(1) {
        // Accept
        int comm_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);
        if (comm_socket > 0) {
            char buff[BUFSIZE];
            int res = 0;
            string received_request = "";

            // Receiving 1 request message
            if ((res = recv(comm_socket, buff, BUFSIZE, 0)) < 0) {
                cerr << "Error receiving from client!" << endl;
                close(comm_socket);
                continue;
            }

            // Call function to create a string to send
            string data_to_send = parseReq(string(buff));

            int size_to_send = data_to_send.length();
            string tmp = "";

            // Sending buffers of data in loop
            // Notice \0 character at the end of C-style string
            while(size_to_send + 1) {
                if (size_to_send > (BUFSIZE - 1)) {  
                    tmp = data_to_send.substr(0, BUFSIZE - 1);
                    data_to_send.erase(0, BUFSIZE - 1);
                    size_to_send = size_to_send - (BUFSIZE - 1);
                }
                else {
                    tmp = data_to_send;
                    size_to_send = -1;
                }
                send(comm_socket, tmp.c_str(), tmp.length() + 1, 0);
                // Receiving OK message
                if (((res = recv(comm_socket, buff, BUFSIZE, 0)) < 0) || strcmp(buff, "OK")) {
                    cerr << "Error receiving OK message!" << endl;
                    size_to_send = -1;
                }
            }
            close(comm_socket);         // Close socket
        }
    }
}

/* Main function 
* Handle parameters and call function listen()
*/
int main(int argc, char const *argv[]) {
    // Handle SIGINT - proper end of activity
    signal(SIGINT, signalHandler);

    // Handle arguments
    int option;
    string port = "";
    opterr = 0;         // Don't print error message if error occur

    while((option = getopt(argc, (char**) argv, "p:")) != -1) {
        switch(option) {
            case 'p':
                if (port.compare("")) {         // Port already set
                    ERR_RET(server_help);
                }
                port = optarg;
                break;
            default:
                ERR_RET(server_help);
        }
    }
    if (!port.compare("")) {
        ERR_RET(server_help);
    }

    if (port.find_first_not_of("0123456789") == string::npos) {     // Check for valid port
        if (stoi(port, NULL, 10) > 65535) {
            ERR_RET("Invalid port!");
        }
    }
    else {
        ERR_RET("Invalid port!");
    }

    listen(port);       // Listen forever or ctrl+c
}
