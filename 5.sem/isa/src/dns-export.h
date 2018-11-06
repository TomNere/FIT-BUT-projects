#ifndef _DNS_EXPORT_H_
#define _DNS_EXPORT_H_

#include <arpa/inet.h>
#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <string.h>
#include <list>
#include <iterator>
#include <stdio.h>

using namespace std;

void pcapHandler(unsigned char*, const struct pcap_pkthdr*, const uint8_t*);

/******************************************************** CONST & MACROS *******************************************************/
#endif