/* Known-answer probe: parse a fixed config with F-Stack's ini parser and print each key it delivers. */
#include <stdio.h>
#include <string.h>
#include "ff_ini_parser.h"

static const char *cfg = "; top comment\n[dpdk]\nlcore_mask = 0x3\nchannel=4\n\n[port0]\naddr = 10.0.0.2 ; inline\nname=\"quoted\"\n";
static size_t pos;

static char *rd(char *str, int num, void *stream)
{
    (void)stream;
    if (pos >= strlen(cfg)) return NULL;
    int n = 0;
    while (n < num - 1 && cfg[pos]) { str[n++] = cfg[pos++]; if (str[n - 1] == '\n') break; }
    str[n] = 0;
    return str;
}

static int h(void *u, const char *s, const char *n, const char *v)
{
    (void)u;
    printf("ini.%s.%s=%s\n", s, n, v);
    return 1;
}

int main(void)
{
    int rc = ini_parse_stream(rd, NULL, h, NULL);
    printf("ini.rc=%d\n", rc);
    return 0;
}
