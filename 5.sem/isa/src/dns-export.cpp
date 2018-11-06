#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <arpa/inet.h>
#include <string.h>
#include <list>
#include <iterator>

#include <err.h>

#include "dns-export.h"
#include "structures.h"
#include "DnsPacket.cpp"
#include "DnsRecord.cpp"

using namespace std;
uint32_t packetCount = 0;
list<DnsRecord> recordList;


/********************************************************** METHODS **********************************************************/

// Write stats to stdout
void writeStats()
{
    list <DnsRecord> :: iterator it; 
        
    for(it = recordList.begin(); it != recordList.end(); it++) 
    {
        cout << it->GetString();
    }
}

// Signal handler
// Write stats end terminate
void handleSig(int sigNum)
{
    writeStats();
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



/*********************************************************** MAIN ***************************************************************/

// Main function
int main(int argc, char const *argv[])
{
    // Handle SIUGSR1
    signal(SIGUSR1, handleSig);

    // Parse arguments
    rawParameters params = parseArg(argc, argv);

    if (params.interface.compare(""))
    {
        logInterface(params.interface, params.syslogServer, params.time);
    }

    if (params.file.compare(""))
    {
        logFile(params.file, params.syslogServer);
    }

    writeStats();
    return 0;
}
