#ifndef _HELPERS_H_
#include "structures.h"

ip_fragment* ipFragAdd(ip_fragment*, ip_fragment*);

char * escape(const uint8_t *, uint32_t, uint32_t,
              uint16_t, uint32_t);

char * mk_error(const char *, const uint8_t *, uint32_t,
                uint16_t );
char * iptostr(ip_addr *);
void print_ts(struct timeval *);
rr_parser_container* findParser(uint16_t, uint16_t);


#endif