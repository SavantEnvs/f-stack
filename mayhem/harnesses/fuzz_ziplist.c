/* Redis ziplist: the deep integrity check Redis runs on RDB / RESTORE payloads (rdb.c, t_hash.c, t_zset.c),
 * then a full walk of the entries it just accepted. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "zmalloc.h"
#include "ziplist.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    unsigned char *zl = zmalloc(size ? size : 1);
    memcpy(zl, data, size);

    if (!ziplistValidateIntegrity(zl, size, 1, NULL, NULL)) {
        zfree(zl);
        return -1;
    }
    for (unsigned char *p = ziplistIndex(zl, 0); p; p = ziplistNext(zl, p)) {
        unsigned char *sval;
        unsigned int slen;
        long long lval;
        ziplistGet(p, &sval, &slen, &lval);
    }
    for (unsigned char *p = ziplistIndex(zl, -1); p; p = ziplistPrev(zl, p)) {
        unsigned char *sval;
        unsigned int slen;
        long long lval;
        ziplistGet(p, &sval, &slen, &lval);
    }
    zfree(zl);
    return 0;
}
