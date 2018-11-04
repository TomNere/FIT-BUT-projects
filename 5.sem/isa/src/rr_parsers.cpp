#include "rr_parsers.h"
#include "helpers.cpp"

using namespace std;

// This is for handling rr's with errors or an unhandled rtype.
rrParserContainer default_rr_parser = {0, 0, escape, NULL, NULL, 0};

// Add them to the list of data parsers in rtypes.c.
extern rrParserContainer rr_parsers[];



// Add parser functions here, they should be prototyped in rtypes.h and
// then defined below.
// Some of the rtypes below use the escape parser.  This isn't
// because we don't know how to parse them, it's simply because that's
// the right parser for them anyway.
rrParserContainer rr_parsers[] =
{
    {1, 1, A, "A", A_DOC, 0},
    {0, 2, domain_name, "NS", D_DOC, 0},
    {0, 5, domain_name, "CNAME", D_DOC, 0},
    {0, 6, soa, "SOA", SOA_DOC, 0},
    {1, 28, AAAA, "AAAA", AAAA_DOC, 0},
    {0, 15, mx, "MX", MX_DOC, 0},
    {0, 46, rrsig, "RRSIG", RRSIG_DOC, 0},
    {0, 16, domain_name, "TXT", NULL_DOC, 0}, 
    {0, 47, nsec, "NSEC", NSEC_DOC, 0},
    {0, 43, ds, "DS", DS_DOC, 0},
    {0, 48, dnskey, "DNSKEY", KEY_DOC, 0}
};
