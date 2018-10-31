#ifndef _PARSERS_H_
#define _PARSERS_H_

#include "structures.h"

uint32_t getTransProt(uint32_t*, const uint8_t);

uint32_t dnsParse(uint32_t, struct pcap_pkthdr*, uint8_t*, dnsInfo*, uint8_t);

uint32_t parseQuestions(uint32_t, uint32_t, struct pcap_pkthdr*, uint8_t*, uint16_t, dnsQuestion**);
uint32_t parseRRSet(uint32_t, uint32_t, struct pcap_pkthdr*, uint8_t*, uint16_t, dnsRR**);
uint32_t parseRR(uint32_t, uint32_t, struct pcap_pkthdr*, uint8_t*, dnsRR*);
                        
#endif                    