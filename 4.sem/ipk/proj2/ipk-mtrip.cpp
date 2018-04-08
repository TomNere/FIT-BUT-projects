#include <iostream>     // I/O operations
#include <string.h>
#include <unistd.h>     // getopt etc.
#include <sys/socket.h> // Sockets...
#include <netinet/in.h> // sockaddr_in...
#include <netdb.h>      // gethostbyname...
#include <pthread.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <sys/time.h>

using namespace std;

struct connection {
    int client_socket;
    struct sockaddr_in server_address;
    string sond;
    struct timeval time_stamp;
    int timeout;
    bool active;
};

struct connection conn;

#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

const string help = "Invalid parameters!\n"
                    "Usage: ./ipk-mtrip reflect -p port \n"
                    "         ./ipk-mtrip meter -h host -p port - s sond_size -t measurement time\n"
                    "\nsond_size(64-1500) is size in bytes of probe packet. 1500B is maximal safe size for UDP.";

void * sending(void *num) {
    long number = (long)num;
    socklen_t serverlen = sizeof(conn.server_address);
    int buff_length = 32 + conn.sond.length() + 1;   // 32 - number '\0' - end

    struct timeval act_time;
    string stable_num;
    for (uint32_t i = 0; ; i++) {
        //cout << "Chcem poslat" << endl;
        if (conn.active == false) {
          //  cout << "Koniec send";
            return (void*)-1;
        }
        /*
        gettimeofday(&act_time, NULL);
        if (((act_time.tv_usec - conn.time_stamp.tv_usec) >= conn.timeout) || 
             ((act_time.tv_sec - conn.time_stamp.tv_sec > 0) && 
               (conn.timeout <= (act_time.tv_usec + (1000000 - conn.time_stamp.tv_usec))))) {
            conn.active = false;
            return (void*)-1;
        }
        */
    
        usleep(number);

        stringstream ss;
        ss << setfill('0') << setw(32) << i;
        ss >> stable_num;
        int bytestx = sendto(conn.client_socket, (stable_num + conn.sond).c_str(), buff_length, 0, (struct sockaddr *) &conn.server_address, serverlen);
        if (bytestx < 0) {
            cerr << "Sendto error!";
            break;
        }
        //cout << "sent" << i << endl;
    }
    return (void*)1;
}

void * receiving(void *num) {
    long number = (long)num;
    socklen_t serverlen = sizeof(conn.server_address);
    struct timeval act_time;

    struct timeval thrash_timeout;
    thrash_timeout.tv_sec = 0;
    thrash_timeout.tv_usec = 20000;     // 20 ms

    struct timeval global_timeout;
    global_timeout.tv_sec = 5;
    global_timeout.tv_usec = 0;

    for (long i = 0; ; i++) {
        gettimeofday(&act_time, NULL);
        if (((act_time.tv_usec - conn.time_stamp.tv_usec) >= conn.timeout) || 
             ((act_time.tv_sec - conn.time_stamp.tv_sec > 0) && 
               (conn.timeout <= (act_time.tv_usec + (1000000 - conn.time_stamp.tv_usec))))) {
            conn.active = false;

            int rest = 0;
            setsockopt(conn.client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&thrash_timeout, sizeof thrash_timeout);
            while(true) {
                char buff[33];
                int bytesrx = recvfrom(conn.client_socket, buff, 33, 0, (struct sockaddr *) &conn.server_address, &serverlen);
                if (bytesrx < 0)
                    break;
                string str = string(buff);
                if (stoi(str.substr(0), NULL, 10) != i) {
                    //cout << "Packet " << i << "lost!" << endl;
                    //cout << buff << endl;
                    while (true) {
                        bytesrx = recvfrom(conn.client_socket, buff, 33, 0, (struct sockaddr *) &conn.server_address, &serverlen);
                        if (bytesrx < 0)
                            break;
                    }
                }
                else {
                    rest++;
                }
            }
            setsockopt(conn.client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&global_timeout, sizeof global_timeout);
            return (void*)(i + rest);
        }
        
        char buff[33];
        int bytesrx = recvfrom(conn.client_socket, buff, 33, 0, (struct sockaddr *) &conn.server_address, &serverlen);
        string str = string(buff);

        if (bytesrx < 0){
            cerr << "Recvfrom error!";
            break;
        }
        if (stoi(str.substr(0), NULL, 10) != i) {
            //cout << "Packet " << i << "lost!" << endl;
            //cout << buff << endl;
            conn.active = false;
            setsockopt(conn.client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&thrash_timeout, sizeof thrash_timeout);
            while (true) {
                bytesrx = recvfrom(conn.client_socket, buff, 33, 0, (struct sockaddr *) &conn.server_address, &serverlen);
                if (bytesrx < 0)
                    break;
                //cout << "Accepted: " << buff << endl;
            }
            setsockopt(conn.client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&global_timeout, sizeof global_timeout);
            return (void*)-2;
        }
    }
    return (void*)1;
}

void reflect(int argc, char const *argv[]) {
    if (argc != 4) {
        ERR_RET(help);
    }

    string port = "";
    int option;
    optind = 2;

    while((option = getopt(argc, (char**) argv, "p:")) != -1) {
        switch(option) {
            case 'p':
                if (port.compare("")) {
                    ERR_RET(help);
                }
                port = string(optarg);
                break;
            default:
                ERR_RET(help);
        }
    }

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR_RET("Socket error!");
    }

    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    struct sockaddr_in server_address;
    int port_number;
    if (port.find_first_not_of("0123456789") == string::npos) {     // Check for valid port
        port_number = stoi(port, NULL, 10);
        if (port_number > 65535) {
            ERR_RET("Invalid port!");
        }
    }
    
    bzero((char *) &server_address, sizeof(server_address)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons((unsigned short)port_number);

    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) { 
        ERR_RET("Bind error!");
    }

    struct sockaddr_in client_address;
    char buff[65000];
    while(true) {
        buff[0] = '\0';
        socklen_t clientlen = sizeof(client_address);
        int bytesrx = recvfrom(server_socket, buff, 65000, 0, (struct sockaddr *) &client_address, &clientlen);

        if (bytesrx < 0)
            cerr << "Recvfrom error!";

        char to_send[33];
        to_send[32] = '\0';
        strncpy(to_send, buff, 32);

        int bytestx = sendto(server_socket, to_send, 33, 0, (struct sockaddr *) &client_address, clientlen);
        if (bytestx < 0)  
            cerr << "Sendto error!"; 
    }
}

void meter(int argc, char const *argv[]) {
    if (argc != 10) {
        ERR_RET(help);
    }
    string host, port, sond_size, time;
    host = port = sond_size = time = "";
    int option;
    optind = 2;

    while((option = getopt(argc, (char**) argv, "h:p:s:t:")) != -1) {
        switch(option) {
            case 'h':
                if (host.compare("")) {
                    ERR_RET(help);
                }
                host = string(optarg);
                break;
            case 'p':
                if (port.compare("")) {
                    ERR_RET(help);
                }
                port = string(optarg);
                break;
            case 's':
                if (sond_size.compare("")) {
                    ERR_RET(help);
                }
                sond_size = string(optarg);
                break;
            case 't':
                if (time.compare("")) {
                    ERR_RET(help);
                }
                time = string(optarg);
                break;
            default:
                ERR_RET(help);
        }
    }

    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR_RET("Socket error!");
    }

    struct hostent* server;
    if ((server = gethostbyname(host.c_str())) == NULL) { 
        ERR_RET("Host not found!")
    }

    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);

    int port_number;
    if (port.find_first_not_of("0123456789") == string::npos) {     // Check for valid port
        port_number = stoi(port, NULL, 10);
        if (port_number > 65535) {
            ERR_RET("Invalid port!");
        }
    }
    server_address.sin_port = htons(port_number);

    int byte_number = 0;
    string sond;
    if (sond_size.find_first_not_of("0123456789") == string::npos) {     // Check for valid sond_size
        byte_number = stoi(sond_size, NULL, 10);
        if (byte_number > 64000 || byte_number < 64) {
            ERR_RET("Sond of this size couldn't be sent!(Range is 64-1500).");
        }
        sond = "&";
        for (int i = 0; i < byte_number - 33; i++) {          // Generate sond
            char c = (i % 25) + 97;
            sond += c;
        }
    }

    int measurement_time;

    if (time.find_first_not_of("0123456789") == string::npos) {     // Check for time interval
        measurement_time = stoi(time, NULL, 10);
        if (measurement_time < 5) {
            cerr << "Warning: Measurement could be inaccurate due to measurement time." << endl;
        }
    }

    // Global timeout for socket(recv_from)
    struct timeval timeout;
    timeout.tv_sec = 5;     // 5 sec
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

    conn.client_socket = client_socket;
    conn.server_address = server_address;
    conn.sond = sond;

    long number = 0;    // Number of microsecond to usleep

    pthread_t thread1, thread2;
    long estimation = 0;
    void* send_ret = 0;
    void* recv_ret = 0;

    // First estimate of bandwidth
    conn.timeout = 500000;                  // 50ms
    cout << "Getting ready (could take longer time with poor network connection and bigger sond_size)." << endl;

    struct timeval est_time;
    gettimeofday(&est_time, NULL);
    
    struct timeval act_time;
    for (int i = 0; ; i++) {
        gettimeofday(&conn.time_stamp, NULL);

        conn.active = true;
        pthread_create(&thread1, NULL, sending, (void *)number);
        pthread_create(&thread2, NULL, receiving, (void *)number);
        pthread_join(thread1, &send_ret);
        pthread_join(thread2, &recv_ret);
        
        if ((long)recv_ret == -2) {
            gettimeofday(&act_time, NULL);
            if ((act_time.tv_sec - est_time.tv_sec) >= 30) {
                estimation = number;
            }
            number += 300;                 // bigger usleep
        }
        else {
            estimation = number;
            break;
        } 
    }
    cout << "\nEstimation" << estimation << endl;
    double max_speed, min_speed, avg_speed;
    max_speed = min_speed = avg_speed = 0;
    double run_speed;
    int measurements = 0;
    double final_speed = 0;
    int succ_runs = 0;

    conn.timeout = 300000;                  // 20ms
    cout << "Starting " << measurement_time << endl;

    gettimeofday(&est_time, NULL);
    for (int i = 0; i < measurement_time; i++) {
        gettimeofday(&conn.time_stamp, NULL);

        conn.active = true;
        pthread_create(&thread1, NULL, sending, (void *)estimation);
        pthread_create(&thread2, NULL, receiving, (void *)estimation);
        pthread_join(thread1, &send_ret);
        pthread_join(thread2, &recv_ret);
        //cout << "recv" << (long)recv_ret << endl;
        
        gettimeofday(&act_time, NULL);
        if ((act_time.tv_sec - est_time.tv_sec) >= 2) {
            //cout << "interval end" << avg_speed << endl;
            run_speed = (avg_speed / measurements) / 1048576;
            final_speed += run_speed;
            succ_runs++;
            cout << "    " << i << run_speed << "Mbit/s" << endl;
            avg_speed = 0;
            measurements = 0;
            gettimeofday(&est_time, NULL);
            continue;
        }

        if ((long)recv_ret == -2) {
            //cout << "crash" << endl;
            estimation += 100;                 // bigger usleep
        }
        else if ((long)recv_ret == 1) {
            //cout << "Continue" << endl;
            continue;
        }
        else{
            //cout << "else " << (long)recv_ret << endl;
            int speed = (long)recv_ret * byte_number * 8 *(10/(3.0));
            if (min_speed == 0) {
                min_speed = speed;
            }
            if (min_speed > speed) {
                min_speed = speed;
            }
            if (max_speed < speed) {
                max_speed = speed;
            }
            avg_speed += speed;
            estimation -= 100;
            measurements++;
        } 
    }
    cout << "Average speed: " << final_speed / succ_runs << "Mbit/s" << endl;
    cout << "Minimal speed: " << min_speed / 1048576 << "Mbit/s" << endl;
    cout << "Maximal speed: " << max_speed / 1048576 << "Mbit/s" << endl;
}

int main(int argc, char const *argv[])
{
    int option;
    opterr = 0;         // Don't print error message if error occur

    if (argc > 1) {
        if (!strcmp(argv[1], "reflect")) {
            reflect(argc, argv);
        }
        else if (!strcmp(argv[1], "meter")) {
            meter(argc, argv);
        }
        else
            ERR_RET(help)
    }
    else
        ERR_RET(help);
    return 0;
}
