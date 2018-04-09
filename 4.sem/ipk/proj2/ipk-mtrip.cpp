#include <iostream>     // I/O operations
#include <string.h>
#include <unistd.h>     // getopt etc.
#include <sys/socket.h> // Sockets...
#include <netinet/in.h> // sockaddr_in...
#include <netdb.h>      // gethostbyname...
#include <pthread.h>
#include <cstdlib>
#include <iomanip>
#include <sys/time.h>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace std;

struct connection {
    int client_socket;
    struct sockaddr_in server_address;
    string sond;
    struct timeval time_stamp;
    struct timeval rtt_stamp;
    double rtt_time;
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
                    "Estimate speed for first run could take longer time with poor network connection and bigger sond_size.\n";

// Interval between 2 time stamps in microseconds
long timeInterval(struct timeval old_time, struct timeval new_time) {
    long gap = 0;
    if (new_time.tv_usec < old_time.tv_usec){
        gap += (new_time.tv_usec + (1000000 - old_time.tv_usec));
        if (new_time.tv_sec - old_time.tv_sec > 1) {
            gap += ((new_time.tv_sec - old_time.tv_sec) - 1) * 1000000;
        }
    }
    else {
        gap += (new_time.tv_sec - old_time.tv_sec) * 1000000;
        gap += (new_time.tv_usec - old_time.tv_usec);
    }
    return gap;
}

// Standart deviation
double stdev(double arr[], int arr_size, double avg_speed) {
    double count = 0;
    for (int i = 0; i < arr_size; i++) {
        count += ((arr[i] - avg_speed) * (arr[i] - avg_speed));
    }
    count = count / (arr_size - 1);         // N - 1
    return sqrt(count);
}

// Thread for sending packets
void* sending(void* par) {
    long number = (long)par;
    socklen_t serverlen = sizeof(conn.server_address);
    int buff_length = conn.sond.length();   // Stable size of packet

    conn.sond[0] = 'R';                     // Packet for RTT
    gettimeofday(&conn.rtt_stamp, NULL);
    int bytestx = sendto(conn.client_socket, conn.sond.c_str(), buff_length, 0, (struct sockaddr *) &conn.server_address, serverlen);
    if (bytestx < 0) {
        cerr << "Sendto error!" << endl;
        return (void*)-1;
    }
    conn.sond[0] = 'a';

    for (long i = 0; ; i++) {
        if (conn.active == false) {         // Send packets until false
            return (void*)(i + 1);          // + 1 - packet for RTT
        }
    
        usleep(number);                     // If too many packets are getting lost - usleep

        int bytestx = sendto(conn.client_socket, conn.sond.c_str(), buff_length, 0, (struct sockaddr *) &conn.server_address, serverlen);
        if (bytestx < 0) {
            cerr << "Sendto error!" << endl;
            break;
        }
    }
    return (void*)-1;
}

// Thread for receiving packets
void* receiving(void* par) {
    socklen_t serverlen = sizeof(conn.server_address);
    struct timeval act_time;

    // Try to get RTT packet
    char buff[ACK_SIZE];
    int bytesrx = recvfrom(conn.client_socket, buff, ACK_SIZE, 0, (struct sockaddr *) &conn.server_address, &serverlen);
    if (bytesrx < 0) {
        cerr << "Recvfrom error!" << endl;
        return (void*)-1;
    }
    gettimeofday(&act_time, NULL);
    if (buff[0] == 'R') {                    // If valid RTT packet, get RTT
        conn.rtt_time = timeInterval(conn.rtt_stamp, act_time);
    }

    for (long i = 0; ; i++) {
        gettimeofday(&act_time, NULL);
        if (timeInterval(conn.time_stamp, act_time) >= conn.timeout) {
            conn.active = false;            // Stop sending after timeout

            int rest = 0;
            while(true) {
                char buff[ACK_SIZE];
                int bytesrx = recvfrom(conn.client_socket, buff, ACK_SIZE, 0, (struct sockaddr *) &conn.server_address, &serverlen);
                if (bytesrx < 0) {
                    break;
                }
                rest++;
            }
            return (void*)(i + 1 + rest);   // + 1 - packet for RTT
        }
        
        char buff[NUMBER_SIZE];
        int bytesrx = recvfrom(conn.client_socket, buff, ACK_SIZE, 0, (struct sockaddr *) &conn.server_address, &serverlen);
        if (bytesrx < 0){
            cerr << "Recvfrom error!" << endl;
            return (void*)-1;
        }
    }
    return (void*)-1;
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

    // Global timeout for socket(recv_from) - depend on size of sond
    struct timeval socket_timeout;
    if (byte_number >= 1000) {
        socket_timeout.tv_sec = 1;     // 1 sec
        socket_timeout.tv_usec = 500 * 1000;   // * 1000 - miliseconds
    }
    else if (byte_number >= 500) {
        socket_timeout.tv_sec = 0;
        socket_timeout.tv_usec = byte_number * 1000;
    }
    else {
        socket_timeout.tv_sec = 0;
        socket_timeout.tv_usec = 500 * 1000;
    }
    
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&socket_timeout, sizeof socket_timeout);
    conn.timeout = socket_timeout.tv_sec * 1000000 + socket_timeout.tv_usec;    // Timeout in microseconds

    // Initialization of struct conn
    conn.client_socket = client_socket;
    conn.server_address = server_address;
    conn.sond = sond;

    pthread_t thread1, thread2;
    long number = 0;            // Number of microsecond to usleep
    long estimation = 0;        // Usleep value
    void* send_ret = 0;         // Return value of sending
    void* recv_ret = 0;         // Return value of receiving

    // First estimate of bandwidth
    cout << "\nGetting ready.";
    cout.flush();               // To print immediately

    struct timeval est_time;        // Ensure that speed estimation won't take longer than 30s
    gettimeofday(&est_time, NULL);
    
    struct timeval act_time;
    for (int i = 0; ; i++) {
        gettimeofday(&conn.time_stamp, NULL);
        conn.active = true;
        pthread_create(&thread1, NULL, sending, (void *)number);
        pthread_create(&thread2, NULL, receiving, (void *)number);
        pthread_join(thread1, &send_ret);
        pthread_join(thread2, &recv_ret);

        gettimeofday(&act_time, NULL);
        if ((act_time.tv_sec - est_time.tv_sec) >= 30) {
            ERR_RET("\nUnable to estimate! Too poor bandwidth for this packet size.");
        }
        if (((long)send_ret - (long)recv_ret) > ((long)send_ret / 100)) {
            number += 50;                 // If lost packet is over 1% , try bigger usleep
            cout << ".";
            cout.flush();
        }
        else {
            estimation = number;
            cout << endl;
            break;
        } 
    }

    int arr_size;           // Size of array where measurement values will be stored

    int loop_limit = 0;     // Number of runs and interval of one run depend on time parameter
    if (time <= 5) {
        cout << "Starting - time interval for one run:  " << "0.5s" << endl;
        socket_timeout.tv_sec = 0;             
        socket_timeout.tv_usec = 500 * 1000;   
        conn.timeout = 500000;
        arr_size = loop_limit = time * 2;
    }
    else if (time <= 8) {
        cout << "\nStarting - time interval for one run:  " << "1s" << endl;
        socket_timeout.tv_sec = 1;     
        socket_timeout.tv_usec = 0;   
        conn.timeout = 1000000;
        arr_size =loop_limit = time;
    }
    else {
        cout << "\nStarting - time interval for one run:  " << "2s" << endl;
        conn.timeout = 2000000;
        loop_limit = time / 2;
        arr_size = loop_limit + (time % 2);
    }
    // Also timeout for socket(recvfrom) is set
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&socket_timeout, sizeof socket_timeout);

    double measurements[arr_size];      // Array of measurement values
    double avg_rtt = 0.0;               // Variable for RTT
    int succ_rtt = 0;                   // Number of successful RTT measurements

    gettimeofday(&est_time, NULL);
    for (int i = 0; i < loop_limit; i++) {
        gettimeofday(&conn.time_stamp, NULL);

        conn.active = true;
        conn.rtt_time = -1;             // Reset RTT

        pthread_create(&thread1, NULL, sending, (void *)estimation);
        pthread_create(&thread2, NULL, receiving, (void *)estimation);
        pthread_join(thread1, &send_ret);
        pthread_join(thread2, &recv_ret);

        if ((long)send_ret == -1 || (long)recv_ret == -1) { // Something went wrong
            recv_ret = 0;                                   // This measurement will be 0 Mbit/s
        }
        else if (((long)send_ret - (long)recv_ret) > ((long)send_ret / 100)) {
            estimation += 50;               // If lost packet is over 1% , try bigger usleep
        }
        else {
            if (estimation >= 50) {         // Else , try smaller usleep if possible
                estimation -= 50;
            }
        }

        if (conn.rtt_time != -1) {              // Successful RTT measurement
            avg_rtt += (conn.rtt_time / 1000);  // Store RTT in miliseconds
            succ_rtt++;
        }

        // Bandwidth in Mbit/s
        measurements[i] = (long)recv_ret * byte_number * 8 * (10.0/(conn.timeout / 100000.0)) / b_to_Mb;

        // Print info
        cout << "\tRun " << i + 1 << "\tpackets: (" << (long)send_ret << "/" << (long)recv_ret << ")"
             << "\t\tspeed: " << measurements[i] << " Mbit/s" << endl;

        // If interval of run is 2, still is time for measurement
        if ((time > 6) && (time % 2 == 1) && (i == loop_limit - 1)) {
            conn.timeout = 1000000;
            loop_limit++;
        }
    }

    // Print final info
    cout << "\n\nMinimal speed: " << *min_element(measurements, measurements + loop_limit) << " Mbit/s" << endl
         << "Maximal speed: " << *max_element(measurements, measurements + loop_limit) << " Mbit/s" << endl;
    double avg_speed = accumulate(measurements, measurements + loop_limit, 0.0) / loop_limit;
    cout << "Average speed: " << avg_speed << " Mbit/s" << endl
         << "STDEV:         " << stdev(measurements, loop_limit, avg_speed) << " Mbit/s" << endl;

    if (succ_rtt == 0) {
        cout << "RTT:           RTT couldn't be measured!" << endl;
    }
    else {
        cout << "RTT:           " << avg_rtt / succ_rtt << " ms" << endl;
    }
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
        int bytestx = sendto(server_socket, buff, ACK_SIZE, 0, (struct sockaddr *) &client_address, clientlen);
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
