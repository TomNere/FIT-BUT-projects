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
#include "structures.h"
#include "DnsData.cpp"

using namespace std;

/******************************************************** CONST & MACROS *******************************************************/
void printSummary(ipInfo*, DnsData*, struct pcap_pkthdr*);
#endif