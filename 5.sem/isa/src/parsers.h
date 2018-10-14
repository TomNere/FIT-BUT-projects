#ifndef _PARSERS_H_
#define _PARSERS_H_

#include "structures.h"

uint32_t ethParse(struct pcap_pkthdr*, uint8_t*, uint16_t*, int);
uint32_t mplsParse(uint32_t, struct pcap_pkthdr*, uint8_t*, uint16_t*);

// The ** to the packet data is passed, instead of the data directly.
// They may set the packet pointer to a new data array.
// On error, the packet pointer is set to NULL.
uint32_t ipv4Parse(uint32_t, struct pcap_pkthdr*, uint8_t**, ip_info*);
uint32_t ipv6Parse(uint32_t, struct pcap_pkthdr*, uint8_t**, ip_info*);

uint32_t udpParse(uint32_t, struct pcap_pkthdr*, uint8_t*, transport_info*);

uint32_t dnsParse(uint32_t, struct pcap_pkthdr*, uint8_t*, dns_info*, uint8_t);

uint32_t parseQuestions(uint32_t, uint32_t, struct pcap_pkthdr*, uint8_t*, uint16_t, dns_question**);

uint32_t parseRRSet(uint32_t, uint32_t, struct pcap_pkthdr*, uint8_t*, uint16_t, dns_rr**);
uint32_t parseRR(uint32_t, uint32_t, struct pcap_pkthdr*, uint8_t*, dns_rr*);

rr_parser_container* findParser(uint16_t, uint16_t);

void sort_parsers();

rr_parser_container* findParser(uint16_t cls, uint16_t rtype);

void print_parsers();

inline int count_parsers();




                        
#endif                    