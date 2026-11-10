/* Redis listpack: lpValidateIntegrity (deep) as run on stream payloads (t_stream.c / rdb.c), then a walk of the entries. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "zmalloc.h"
#include "listpack.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    unsigned char *lp = zmalloc(size ? size : 1);
    memcpy(lp, data, size);

    if (!lpValidateIntegrity(lp, size, 1)) {
        zfree(lp);
        return -1;
    }
    unsigned char intbuf[LP_INTBUF_SIZE];
    for (unsigned char *p = lpFirst(lp); p; p = lpNext(lp, p)) {
        int64_t n;
        lpGet(p, &n, intbuf);
    }
    for (unsigned char *p = lpLast(lp); p; p = lpPrev(lp, p)) {
        int64_t n;
        lpGet(p, &n, intbuf);
    }
    zfree(lp);
    return 0;
}
