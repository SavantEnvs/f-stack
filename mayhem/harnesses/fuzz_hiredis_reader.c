/* hiredis RESP reader (Redis deps/hiredis/read.c): the parser redis-cli and every hiredis client runs on server bytes. */
#include <stddef.h>
#include <stdint.h>
#include "hiredis.h"
#include "read.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    redisReader *r = redisReaderCreate();
    if (!r) return -1;
    if (redisReaderFeed(r, (const char *)data, size) != REDIS_OK) {
        redisReaderFree(r);
        return -1;
    }
    int result = 0;
    for (;;) {
        void *reply = NULL;
        if (redisReaderGetReply(r, &reply) != REDIS_OK) { result = -1; break; }
        if (reply == NULL) break;
        freeReplyObject(reply);
    }
    redisReaderFree(r);
    return result;
}
