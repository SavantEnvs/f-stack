/* Known-answer probe: parse a fixed RESP stream with the hiredis reader and print the decoded replies. */
#include <stdio.h>
#include <string.h>
#include "hiredis.h"
#include "read.h"

int main(void)
{
    const char *wire = "*3\r\n$3\r\nfoo\r\n:42\r\n-ERR bad\r\n+OK\r\n$-1\r\n";
    redisReader *r = redisReaderCreate();
    redisReaderFeed(r, wire, strlen(wire));
    void *rep = NULL;
    int n = 0;
    while (redisReaderGetReply(r, &rep) == REDIS_OK && rep) {
        redisReply *x = rep;
        printf("hiredis.reply%d.type=%d\n", n, x->type);
        if (x->type == REDIS_REPLY_ARRAY) {
            printf("hiredis.reply%d.elements=%zu\n", n, x->elements);
            printf("hiredis.reply%d.e0=%s\n", n, x->element[0]->str);
            printf("hiredis.reply%d.e1=%lld\n", n, x->element[1]->integer);
            printf("hiredis.reply%d.e2=%s\n", n, x->element[2]->str);
        } else if (x->type == REDIS_REPLY_STATUS) {
            printf("hiredis.reply%d.str=%s\n", n, x->str);
        }
        freeReplyObject(rep);
        rep = NULL;
        n++;
    }
    printf("hiredis.count=%d\n", n);
    redisReader *bad = redisReaderCreate();
    redisReaderFeed(bad, "?junk\r\n", 7);
    int rc = redisReaderGetReply(bad, &rep);
    printf("hiredis.protocol_error=%d\n", rc == REDIS_ERR);
    redisReaderFree(bad);
    redisReaderFree(r);
    return 0;
}
