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
#include <algorithm>
#include <numeric>
#include <math.h>
#include <array>

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

#define MAX_BUFF_SIZE 64000
#define MIN_BUFF_SIZE 64
#define NUMBER_SIZE 16
#define ACK_SIZE 1
#define PORT_RANGE 65535
#define b_to_Mb 1048576

const string help = "Invalid parameters!\n"
                    "Usage: ./ipk-mtrip reflect -p port \n"
                    "         ./ipk-mtrip meter -h host -p port - s sond_size -t measurement time\n"
                    "\nsond_size(64-64000) is size in bytes of probe packet. 1500B is maximal safe size for UDP."
                    "Aproximate speed for first run could take longer time with poor network connection and bigger sond_size.\n";

void * sending(void *num) {
    long number = (long)num;
    socklen_t serverlen = sizeof(conn.server_address);
    int buff_length = conn.sond.length();   // 16 - number

    struct timeval act_time;
    string stable_num;
    for (long i = 0; ; i++) {
        //cout << "Chcem poslat" << endl;
        if (conn.active == false) {
            //cout << "timeout_sec" << conn.time_stamp.tv_sec <<endl;
            //cout << "timeout_usec" << conn.time_stamp.tv_usec <<endl;
          //  cout << "Koniec send";1
            //cout << "send false" << endl;
            return (void*)(i);
        }
    
        usleep(number);

        int bytestx = sendto(conn.client_socket, conn.sond.c_str(), buff_length, 0, (struct sockaddr *) &conn.server_address, serverlen);
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
        int gap = 0;
        if (act_time.tv_usec < conn.time_stamp.tv_usec){
            gap += (act_time.tv_usec + (1000000 - conn.time_stamp.tv_usec));
            if (act_time.tv_sec - conn.time_stamp.tv_sec > 1) {
                gap += ((act_time.tv_sec - conn.time_stamp.tv_sec) - 1) * 1000000;
            }
        }
        else {
            gap += (act_time.tv_sec - conn.time_stamp.tv_sec) * 1000000;
            gap += (act_time.tv_usec - conn.time_stamp.tv_usec);

        }

        if (gap >= conn.timeout) {
            conn.active = false;
            //cout << "timeout_sec" << act_time.tv_sec <<endl;
            //cout << "timeout_usec" << act_time.tv_usec <<endl;
            int rest = 0;
            while(true) {
                char buff[ACK_SIZE];
                int bytesrx = recvfrom(conn.client_socket, buff, ACK_SIZE, 0, (struct sockaddr *) &conn.server_address, &serverlen);
                if (bytesrx < 0) {
                    break;
                }
                rest++;
            }
            return (void*)(i + rest);
        }
        
        char buff[NUMBER_SIZE];
        int bytesrx = recvfrom(conn.client_socket, buff, ACK_SIZE, 0, (struct sockaddr *) &conn.server_address, &serverlen);
        //cout << buff << endl;

        if (bytesrx < 0){
            cerr << "Recvfrom error!";
            return (void*)i;
        }
    }
    return (void*)1;

}

double stdev(double arr[], int arr_size, double avg_speed) {
    double count = 0;
    for (int i = 0; i < arr_size; i++) {
        count += ((arr[i] - avg_speed) * (arr[i] - avg_speed));
    }
    count = count / (arr_size - 1);         // N - 1
    count = sqrt(count);
    return count;
}

void meter(int argc, char const *argv[]) {
    if (argc != 10) {
        ERR_RET(help);
    }
    string par_host, par_port, par_sond, par_time;
    par_host = par_port = par_sond = par_time = "";
    int option;
    optind = 2;             // First parameter after "meter"

    while((option = getopt(argc, (char**) argv, "h:p:s:t:")) != -1) {
        switch(option) {
            case 'h':
                if (par_host.compare("")) {
                    ERR_RET(help);
                }
                par_host = string(optarg);
                break;
            case 'p':
                if (par_port.compare("")) {
                    ERR_RET(help);
                }
                par_port = string(optarg);
                break;
            case 's':
                if (par_sond.compare("")) {
                    ERR_RET(help);
                }
                par_sond = string(optarg);
                break;
            case 't':
                if (par_time.compare("")) {
                    ERR_RET(help);
                }
                par_time = string(optarg);
                break;
            default:
                ERR_RET(help);
        }
    }

    /*************************************SOCKET STUFF********************************************/

    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR_RET("Socket error!");
    }

    struct hostent* server;
    if ((server = gethostbyname(par_host.c_str())) == NULL) { 
        ERR_RET("Host not found!")
    }

    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);

    int port_number;
    if (par_port.find_first_not_of("0123456789") == string::npos) {     // Check for valid port
        port_number = stoi(par_port, NULL, 10);
        if (port_number > PORT_RANGE) {                  // Port range
            ERR_RET("Invalid port!");
        }
    }
    server_address.sin_port = htons(port_number);

    int byte_number = 0;
    string sond;
    if (par_sond.find_first_not_of("0123456789") == string::npos) {     // Check for valid sond 
        byte_number = stoi(par_sond, NULL, 10);
        if (byte_number > MAX_BUFF_SIZE || byte_number < MIN_BUFF_SIZE) {
            ERR_RET("Sond of this size couldn't be sent!(Range is 64-1500).");
        }
        sond = "";
        for (int i = 0; i < byte_number; i++) {          // Generate sond
            char c = (i % 25) + 97;
            sond += c;
        }
    }
    else {
        ERR_RET("Sond must be number!");
    }

    int time;
    if (par_time.find_first_not_of("0123456789") == string::npos) {     // Check for time interval
        time = stoi(par_time, NULL, 10);
        if (time < 3) {
            ERR_RET("Invalid time interval for measurement!(Must be at least 3).")
        }
    }
    else {
        ERR_RET("Time must be number!");
    }

    /****************************************PREPARE FOR MEASUREMENT*****************************************/

    // Global timeout for socket(recv_from)
    struct timeval socket_timeout;

    if (byte_number >= 1000) {
        socket_timeout.tv_sec = 1;     // 1 sec
        socket_timeout.tv_usec = 500 * 1000;   // Miliseconds
    }
    else if (byte_number >= 500){
        socket_timeout.tv_sec = 0;
        socket_timeout.tv_usec = byte_number * 1000;
    }
    else {
        socket_timeout.tv_sec = 0;
        socket_timeout.tv_usec = 500 * 1000;
    }
    
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&socket_timeout, sizeof socket_timeout);
    conn.timeout = socket_timeout.tv_sec * 1000000 + socket_timeout.tv_usec;                  // 50ms

    // Initialization of struct conn
    conn.client_socket = client_socket;
    conn.server_address = server_address;
    conn.sond = sond;

    long number = 0;    // Number of microsecond to usleep
    pthread_t thread1, thread2;
    long estimation = 0;
    void* send_ret = 0;
    void* recv_ret = 0;

    // First estimate of bandwidth
    cout << "\nGetting ready.";
    cout.flush();

    struct timeval est_time;
    gettimeofday(&est_time, NULL);
    
    struct timeval act_time;
    for (int i = 0; ; i++) {
        gettimeofday(&conn.time_stamp, NULL);

        conn.active = true;
        pthread_create(&thread1, NULL, sending, (void *)number);
        pthread_create(&thread2, NULL, receiving, (void *)number);
        pthread_join(thread2, &recv_ret);
        pthread_join(thread1, &send_ret);
        //cout << "sent " << (long)send_ret << endl;
        //cout << "recv " << (long)recv_ret << endl;

        gettimeofday(&act_time, NULL);
        if ((act_time.tv_sec - est_time.tv_sec) >= 30) {
            ERR_RET("\nUnable to estimate! Too poor bandwidth for this packet size.");
        }

        if (((long)send_ret - (long)recv_ret) > ((long)send_ret / 100)) {
            number += 50;                 // bigger usleep
            cout << ".";
            cout.flush();
        }
        else {
            estimation = number;
            cout << endl;
            break;
        } 
    }

    //cout << "\nEstimation usleep: " << estimation << endl;

    double run_speed;
    int arr_size;

    int loop_limit = 0;
    if (time <= 5) {
        cout << "Warning: Measurement could be innaccurate due to time!" << endl;
        cout << "Time interval for one run: " << "0.5s" << endl;
        socket_timeout.tv_sec = 0;     // 1 sec
        socket_timeout.tv_usec = 500 * 1000;   // Miliseconds
        conn.timeout = 500000;
        arr_size = loop_limit = time * 2;
    }
    else if (time <= 8) {
        cout << "Time interval for one run: " << "1s" << endl;
        socket_timeout.tv_sec = 1;     // 1 sec
        socket_timeout.tv_usec = 0;   // Miliseconds
        conn.timeout = 1000000;
        arr_size =loop_limit = time;
    }
    else {
        cout << "Time interval for one run: " << "2s" << endl;
        conn.timeout = 2000000;
        loop_limit = time / 2;
        arr_size = loop_limit + (time % 2);
    }
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&socket_timeout, sizeof socket_timeout);
    double measurements[arr_size];

    cout << "\nStarting..." << endl << endl;

    gettimeofday(&est_time, NULL);
    for (int i = 0; i < loop_limit; i++) {
        gettimeofday(&conn.time_stamp, NULL);
        //cout << "act_sec" << conn.time_stamp.tv_sec <<endl;
        //cout << "act_usec" << conn.time_stamp.tv_usec <<endl;

        conn.active = true;
        pthread_create(&thread1, NULL, sending, (void *)estimation);
        pthread_create(&thread2, NULL, receiving, (void *)estimation);
        pthread_join(thread2, &recv_ret);
        pthread_join(thread1, &send_ret);
        //cout << "recv" << (long)recv_ret << endl;

        if (((long)send_ret - (long)recv_ret) > ((long)send_ret / 100)) {
            //cout << "crash" << endl;
            estimation += 50;                 // bigger usleep
        }
        else {
            if (estimation >= 50) {
                estimation -= 50;
            }
        }

        measurements[i] = (long)recv_ret * byte_number * 8 *(10.0/(conn.timeout / 100000.0)) / b_to_Mb;
        cout << "Run " << i + 1 << "\tpackets: (" << (long)send_ret << "/" << (long)recv_ret << ")" << endl;
        cout << "      \tspeed: " << measurements[i] << " Mbit/s" << endl << endl;
        if ((time > 6) && (time % 2 == 1) && (i == loop_limit - 1)) {
            conn.timeout = 1000000;
            loop_limit++;
        }
    }

    cout << "Minimal speed: " << setprecision(5) << *min_element(measurements, measurements + loop_limit) << " Mbit/s" << endl
         << "Maximal speed: " << *max_element(measurements, measurements + loop_limit) << " Mbit/s" << endl;
    double avg_speed = accumulate(measurements, measurements + loop_limit, 0.0) / loop_limit;
    cout << "Average speed: " << setprecision(5) << avg_speed << " Mbit/s" << endl
         << "STDEV:         " << stdev(measurements, loop_limit, avg_speed) << " Mbit/s" << endl;
}

void reflect(int argc, char const *argv[]) {
    if (argc != 4) {
        ERR_RET(help);
    }

    string par_port = "";
    int option;
    optind = 2;     // First parameter after "reflect"

    while((option = getopt(argc, (char**) argv, "p:")) != -1) {
        switch(option) {
            case 'p':
                if (par_port.compare("")) {
                    ERR_RET(help);
                }
                par_port = string(optarg);
                break;
            default:
                ERR_RET(help);
        }
    }

    /****************************SOCKET STUFF***************************************************/

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR_RET("Socket error!");
    }

    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    struct sockaddr_in server_address;
    int port_number;
    if (par_port.find_first_not_of("0123456789") == string::npos) {     // Check for valid port
        port_number = stoi(par_port, NULL, 10);
        if (port_number > PORT_RANGE) {                  // Port range
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

    /************************************INFINITE LOOP**********************************************************/

    // Receive and send answer until ctrl+c
    while(true) {
        char buff[MAX_BUFF_SIZE];

        socklen_t clientlen = sizeof(client_address);
        int bytesrx = recvfrom(server_socket, buff, MAX_BUFF_SIZE, 0, (struct sockaddr *) &client_address, &clientlen);

        if (bytesrx < 0) {
            cerr << "Recvfrom error!";
            continue;
        }

        // Send only ACK packet
        int bytestx = sendto(server_socket, "!", ACK_SIZE, 0, (struct sockaddr *) &client_address, clientlen);
        if (bytestx < 0)  
            cerr << "Sendto error!"; 
    }
}

int main(int argc, char const *argv[]) {
    // Select reflect or meter
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
