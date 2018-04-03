#include <iostream>     // I/O operations
#include <string.h>
#include <unistd.h>     // getopt etc.   

using namespace std;

#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);
const string help = "Nespravna kombinace parametru!\n"
                    "Pouziti: ./ipk-mtrip reflect -p port \n"
                    "         ./ipk-mtrip meter -h vzdáleny_host -p vzdálený_port - s velikost_sondy -t doba_mereni\n";

void reflect(int argc, char const *argv[]) {
    if (argc != 4) {
        ERR_RET(help);
    }

    string port = "";

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

void meter(int argc, char const *argv[]) {
    if (argc != 10) {
        ERR_RET(help);
    }

    string host, port, sond_size, time;
    host = port = sond_size = time = "";
    int option;

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
    cout << host << endl;
    cout << port << endl;
    cout << sond_size << endl;
    cout << time << endl;
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
