/* F-Stack's config-file parser (lib/ff_ini_parser.c, inih): the parser behind ff_load_config's `--conf` file. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "ff_ini_parser.h"

struct mem_stream {
    const uint8_t *data;
    size_t size, pos;
};

static char *mem_fgets(char *str, int num, void *stream)
{
    struct mem_stream *m = stream;
    if (m->pos >= m->size || num <= 1) return NULL;
    int n = 0;
    while (n < num - 1 && m->pos < m->size) {
        char c = (char)m->data[m->pos++];
        str[n++] = c;
        if (c == '\n') break;
    }
    str[n] = '\0';
    return str;
}

static int handler(void *user, const char *section, const char *name, const char *value)
{
    size_t *seen = user;
    *seen += strlen(section) + strlen(name) + strlen(value);
    return 1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct mem_stream m = { data, size, 0 };
    size_t seen = 0;
    return ini_parse_stream(mem_fgets, &m, handler, &seen) ? -1 : 0;
}
