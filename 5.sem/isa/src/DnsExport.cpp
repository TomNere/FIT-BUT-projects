#include <unistd.h> // Getopt
#include <list>
#include <iostream> // No comment needed...
#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <iterator>
#include <sys/socket.h> // Sockets
#include <netdb.h>      // gethostbyname...
#include <ctime>

#include <err.h>

#include "dns-export.h"
#include "structures.h"
#include "DnsPacket.cpp"
#include "DnsRecord.cpp"

using namespace std;

class DnsExport
{
    /*********************************************** PRIVATE Variables ********************************************/
    
    // Parameters
    string file;
    string interface;
    string syslogServer;
    string time;

    // For debugging
    uint32_t packetCount = 0;

    // List collecting all stats to print/send
    list<DnsRecord> recordList;

    // Pcap stuff
    pcap_t* myPcap;
    char errBuf[PCAP_ERRBUF_SIZE];


    /*********************************************** PRIVATE Methods ********************************************/

    // Parse arguments
    void parseArg(int argc, char const *argv[])
    {
        this->file = this->interface = this->syslogServer = this->time = "";
        int option;

        while ((option = getopt(argc, (char **)argv, "r:i:s:t:")) != -1)
        {
            switch (option)
            {
                case 'r':
                    if (this->file.compare(""))
                    {
                        ERR_RET(help);
                    }

                    this->file = string(optarg);
                    break;
                case 'i':
                    if (this->interface.compare(""))
                    {
                        ERR_RET(help);
                    }

                    this->interface = string(optarg);
                    break;
                case 's':
                    if (this->syslogServer.compare(""))
                    {
                        ERR_RET(help);
                    }

                    this->syslogServer = string(optarg);
                    break;
                case 't':
                    if (this->time.compare(""))
                    {
                        ERR_RET(help);
                    }

                    this->time = string(optarg);
                    break;
                default:
                    ERR_RET(help);
            }
        }
    }

    // Signal handler for SIGUSR1 writing stats to stdout
    void handleSig(int sigNum)
    {
        this->writeStats();
    }

    // Write stats to stdout
    void writeStats()
    {
        list <DnsRecord> :: iterator it; 

        for(it = recordList.begin(); it != recordList.end(); it++) 
        {
            cout << it->GetString();
        }
    }

    // Stats sender
    void statsSender()
    {
        int time;
        if (!(this->time.compare("")))
        {
            time = 60; // Default value
        }
        else
        {
            // Check for valid time
            if (this->time.find_first_not_of("0123456789") == string::npos)
            {
                time = stoi(this->time, NULL, 10);
            }
            else
            {
                ERR_RET("Time must be number in seconds!");
            }
        }
    }

    // Interface sniffer
    void logInterface()
    {
        // Open device for sniffing
        if ((this->myPcap = pcap_open_live(this->interface.c_str(), BUFSIZ, true, 0, this->errBuf)) == NULL)
        {
            ERR_RET("Unable to open interface for listening. Error: " << errBuf);
        }

        // Start sniffing forever...
        if (pcap_loop(this->myPcap, -1, this->pcapHandler, NULL) < 0)
        {
            pcap_close(this->myPcap);
            ERR_RET("Error occured when sniffing.");
        }
    }

    // File sniffer
    void logFile()
    {
        // Open file for sniffing
        if ((this->myPcap = pcap_open_offline(this->file.c_str(), this->errBuf)) == NULL)
        {
            ERR_RET("Unable to open pcap file. Error:" << errBuf);
        }

        // Sniff file
        if (pcap_loop(this->myPcap, -1, this->pcapHandler, NULL) < 0)
        {
            pcap_close(this->myPcap);
            ERR_RET("Error occured when reading pcap file.");
        }

        pcap_close(this->myPcap);

        // Send stats if syslog server defined, write to stdout otherwise
        if (!this->syslogServer.compare(""))
        {
            this->writeStats();
        }
        else
        {
            this->sendStats();
        }
    }

    // This callback is called in pcap_loop for each packet
    // Parse packet and add stats if valid and supported DNS packet
    void pcapHandler(unsigned char* useless, const struct pcap_pkthdr* origHeader, const uint8_t* origPacket)
    {
        LOGGING("Handling packet " << packetCount++);
        // if (packetCount != 3)
        // {
        //     return;
        // }

        uint32_t currentPos = 0;


        DnsPacket packet(origPacket, *origHeader);
        packet.Parse();

        list <DnsRR> :: iterator it; 

        for (it = packet.dns.answers.begin(); it != packet.dns.answers.end(); it++) 
        {
            list <DnsRecord> :: iterator it2; 

            bool added = false;
            for (it2 = recordList.begin(); it2 != recordList.end(); it2++) 
            {
                if (it2->IsEqual(it->name, it->type, it->data))
                {
                    it2->AddRecord();
                    added = true;
                    break;
                }
            }

            if (!added)
            {
                DnsRecord dnsR(it->name, it->type, it->rrName, it->data);
                recordList.push_front(dnsR);
            }
        }
    }

    void sendStats(string strLog)
    {
        // Create socket
        int sock;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
        {
            ERR_RET("Syslog - socket creating error!")
        }

        // Set timeout
        // (https://stackoverflow.com/questions/30395258/setting-timeout-to-recv-function)
        struct timeval tv;
        tv.tv_sec = 5;          // 5 Secs Timeout
        tv.tv_usec = 0;         // Not init'ing this can cause strange errors
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

        LOGGING("Server address: " << strLog);

        // Get network host entry
        struct hostent* server;
        if ((server = gethostbyname(strLog.c_str())) == NULL)
        {
            ERR_RET("Syslog - host not found!");
        }

        // Create necessary structures
        struct sockaddr_in server_address;
        bzero((char*) &server_address, sizeof(server_address));
        server_address.sin_family = AF_INET;
        bcopy((char*)server->h_addr, (char*)&server_address.sin_addr.s_addr, server->h_length);
        server_address.sin_port = htons(SYSLOG_PORT);        // Port is string, need conversion
        LOGGING("Syslog address: " << server_address.sin_addr.s_addr << " port:" << server_address.sin_port);
        // Connect
        if ((connect(sock, (const struct sockaddr*)&server_address, sizeof(server_address))) != 0)
        {
            ERR_RET("Syslog - connection error!");
        }

        // Get and send mesagges
        list<string>messages = getMessages();
        list<string>::iterator it;

        for (it = messages.begin(); it != messages.end(); it++)
        {
            int bytestx = send(sock, it->c_str(), it->length() + 1, 0);      // + 1 for '\0' character 
            if (bytestx < 0)
            {
                ERR_RET("Syslog - sending error!");
            }
        }
    
        close(sock);           // Close socket
    }

    list<string> getMessages()
    {
        list<string> messages = {""};
        string message = "";

        string hostname;
        char tmp [1024] = "";
        const int result = gethostname(tmp, sizeof tmp);
        hostname = tmp;

        // Create iterator pointing to first element
        list<DnsRecord>::iterator it;

        for (it = recordList.begin(); it != recordList.end(); it++)
        {
            string record = it->GetString();

            if ((record + message).size() > MESSAGE_SIZE)
            {
                message = "<134>1 " + getFormattedTime() + " " + hostname + " dns-export - - - " + message;
                messages.push_front(message);
                message = "";
            }
            message += record;
        }
        message = "<134>1 " + getFormattedTime() + " " + hostname + " dns-export - - - " + message;
        messages.push_front(message);

        return messages;
    }

    // Returns the local date/time formatted as 2014-03-19 11:11:52
    // https://stackoverflow.com/questions/7411301/how-to-introduce-date-and-time-in-log-file
    string getFormattedTime()
    {
        timeval curTime;
        gettimeofday(&curTime, NULL);
        int milli = curTime.tv_usec / 1000;

        char buffer [80];
        strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", localtime(&curTime.tv_sec));

        char currentTime[84] = "";
        sprintf(currentTime, "%s.%03dZ", buffer, milli);
        return currentTime;

        // time_t rawtime;
        // struct tm* timeinfo;

        // time(&rawtime);
        // timeinfo = localtime(&rawtime);
        // int milli = timeinfo->tv_usec / 1000;

        // // Must be static, otherwise won't work
        // static char _retval[20];
        // strftime(_retval, sizeof(_retval), "%Y-%m-%dT%H:%M:%S", timeinfo);

        // return _retval;
    }


    /*********************************************** PUBLIC Methods ********************************************/

    public:
        // Only parsing arguments in constructor
        DnsExport(int argc, char const *argv[])
        {
            this->parseArg(argc, argv);
        }

        // Main method
        int Main()
        {
            // Handle SIUGSR1
            signal(SIGUSR1, this->handleSig);

            // Choose what to do
            if (this->interface.compare(""))
            {
                //pthread_t t;
                //pthread_create(&t, NULL, call_from_thread, NULL);

                this->logInterface();
                //pthread_join(t, NULL);

            }

            if (this->file.compare(""))
            {
                this->logFile(this->file, this->syslogServer);
            }

            return 0;
        }   
}  