/* Redis intset: intsetValidateIntegrity (deep) as run on RDB / RESTORE payloads (rdb.c), then lookups on the accepted set. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "zmalloc.h"
#include "intset.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    unsigned char *is = zmalloc(size ? size : 1);
    memcpy(is, data, size);

    if (!intsetValidateIntegrity(is, size, 1)) {
        zfree(is);
        return -1;
    }
    intset *set = (intset *)is;
    uint32_t len = intsetLen(set);
    for (uint32_t i = 0; i < len; i++) {
        int64_t v;
        if (intsetGet(set, i, &v)) intsetFind(set, v);
    }
    intsetBlobLen(set);
    zfree(is);
    return 0;
}
