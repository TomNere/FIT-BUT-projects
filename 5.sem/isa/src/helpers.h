#ifndef _HELPERS_H_
#define _HELPERS_H_
#include "structures.h"

char* escape(const uint8_t *, uint32_t, uint32_t,
              uint16_t, uint32_t);

string escape_data(const uint8_t*, uint32_t, uint32_t);
char* iptostr(ipAddr *);
void print_ts(struct timeval *);
rrParserContainer* findParser(uint16_t, uint16_t);

#endif