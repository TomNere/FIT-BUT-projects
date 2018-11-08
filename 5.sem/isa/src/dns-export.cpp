#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <arpa/inet.h>
#include <string.h>
#include <list>
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

/********************************************************** GLOBALS *************************************************************/

uint32_t packetCount = 0;
list<DnsRecord> recordList;

/*********************************************************** MAIN ***************************************************************/

// void* call_from_thread(void*)
// {
//     while(true)
//     {
//         usleep(1000000);
//         cout << "Launched by thread" << endl;
//         writeStats();
//     }
    
//     return NULL;
// }

// Main function
int main(int argc, char const *argv[])
{
    // Handle SIUGSR1
    signal(SIGUSR1, handleSig);

    // Parse arguments
    rawParameters params = parseArg(argc, argv);

    if (params.interface.compare(""))
    {
         //pthread_t t;
         //pthread_create(&t, NULL, call_from_thread, NULL);

        logInterface(params.interface, params.syslogServer, params.time);
         //pthread_join(t, NULL);
        
    }

    if (params.file.compare(""))
    {
        logFile(params.file, params.syslogServer);
    }

    return 0;
}

/********************************************************** METHODS **********************************************************/

// Signal handler
// Write stats end terminate
void handleSig(int sigNum)
{
    writeStats();
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

string getHostname()
{
    char name [1024] = "";
    const int result = gethostname(name, sizeof name);
    LOGGING("hostname result: " << result);
    return name;
}

list<string> getMessages()
{
    list<string> messages = {""};
    string message = "";

    // Create iterator pointing to first element
    list<DnsRecord>::iterator it;

    for (it = recordList.begin(); it != recordList.end(); it++)
    {
        string record = it->GetString();

        if ((record + message).size() > MESSAGE_SIZE)
        {
            message = "<134>1 " + getFormattedTime() + " " + getHostname() + " dns-export - - - " + message;
            messages.push_front(message);
            message = "";
        }
        message += record;
    }
    message = "<134>1 " + getFormattedTime() + " " + getHostname() + " dns-export - - - " + message;
    messages.push_front(message);
    
    return messages;
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

// Argument parser
rawParameters parseArg(int argc, char const *argv[])
{
    rawParameters params;
    params.file = params.interface = params.syslogServer = params.time = "";
    int option;

    while ((option = getopt(argc, (char **)argv, "r:i:s:t:")) != -1)
    {
        switch (option)
        {
        case 'r':
            if (params.file.compare(""))
            {
                ERR_RET(help);
            }

            params.file = string(optarg);
            break;
        case 'i':
            if (params.interface.compare(""))
            {
                ERR_RET(help);
            }

            params.interface = string(optarg);
            break;
        case 's':
            if (params.syslogServer.compare(""))
            {
                ERR_RET(help);
            }

            params.syslogServer = string(optarg);
            break;
        case 't':
            if (params.time.compare(""))
            {
                ERR_RET(help);
            }

            params.time = string(optarg);
            break;
        default:
            ERR_RET(help);
        }
    }

    return params;
}

/*********************************************************** INTERFACE ***************************************************************/

void logInterface(string strInterface, string strLog, string strTime)
{
    int time;
    if (!strTime.compare(""))
    {
        time = 60; // Default value
    }
    else
    {
        // Check for valid time
        if (strTime.find_first_not_of("0123456789") == string::npos)
        {
            time = stoi(strTime, NULL, 10);
        }
        else
        {
            ERR_RET("Time must be number in seconds!");
        }
    }

    char errBuf[PCAP_ERRBUF_SIZE];
    pcap_t* pcapInterface;

    /* Define the device */
		char* dev = pcap_lookupdev(errBuf);
		if (dev == NULL) {
			fprintf(stderr, "Couldn't find default device: %s\n", errBuf);
			return;
		}

    //if ((pcapInterface = pcap_open_live(strInterface.c_str(), 1, 1000, 1000))
    if ((pcapInterface = pcap_open_live(dev, BUFSIZ, true, time * 100, errBuf)) == NULL)
    {
        ERR_RET("Unable to open interface for listening. Error: " << errBuf);
    }

    if (pcap_loop(pcapInterface, -1, pcapHandler, NULL) < 0)
    {
        pcap_close(pcapInterface);
        ERR_RET("Error occured when listening on interface.");
    }

    pcap_close(pcapInterface);
}

void logFile(string strFile, string strLog)
{
    LOGGING("Logging from file");

    char errBuf[PCAP_ERRBUF_SIZE];
    pcap_t* pcapFile;

    if ((pcapFile = pcap_open_offline(strFile.c_str(), errBuf)) == NULL)
    {
        ERR_RET("Unable to open pcap file. Error:" << errBuf);
    }

    if (pcap_loop(pcapFile, -1, pcapHandler, NULL) < 0)
    {
        pcap_close(pcapFile);
        ERR_RET("Error occured when reading pcap file.");
    }

    pcap_close(pcapFile);
    
    if (!strLog.compare(""))
    {
        writeStats();
    }
    else
    {
        sendStats(strLog);
    }
}

/*********************************************************** PCAP FILE ***************************************************************/

// This method is called in pcap_loop for each packet
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
