#ifndef _DNS_EXPORT_H_
#define _DNS_EXPORT_H_

#include <unistd.h> // Getopt
#include <pcap.h>   // No comment needed...
#include <string.h>

using namespace std;

/******************************************************** STRUCTURES ************************************************************/

// Structure representing parameters of running program in raw string format
typedef struct
{
    string file;
    string interface;
    string syslogServer;
    string time;
} rawParameters;

void handleSig(int);
void writeStats();
void sendStats(string);
list<string> getMessages();
rawParameters parseArg(int, char const**argv);
void logInterface(string, string, string);
void logFile(string, string);
void pcapHandler(unsigned char*, const struct pcap_pkthdr*, const uint8_t*);

#endif