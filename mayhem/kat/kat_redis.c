/* Known-answer probe: build ziplist / listpack / intset values with Redis's own encoders, validate them,
 * and read them back. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "zmalloc.h"
#include "ziplist.h"
#include "listpack.h"
#include "intset.h"

int main(void)
{
    unsigned char *zl = ziplistNew();
    zl = ziplistPush(zl, (unsigned char *)"hello", 5, ZIPLIST_TAIL);
    zl = ziplistPush(zl, (unsigned char *)"12345", 5, ZIPLIST_TAIL);
    zl = ziplistPush(zl, (unsigned char *)"x", 1, ZIPLIST_HEAD);
    printf("ziplist.len=%u\n", ziplistLen(zl));
    printf("ziplist.valid=%d\n", ziplistValidateIntegrity(zl, ziplistBlobLen(zl), 1, NULL, NULL));
    unsigned char *sval = NULL; unsigned int slen; long long lval;
    unsigned char *p = ziplistIndex(zl, 2);
    if (p && ziplistGet(p, &sval, &slen, &lval) && !sval) printf("ziplist.e2=%lld\n", lval);
    p = ziplistIndex(zl, 1);
    if (p && ziplistGet(p, &sval, &slen, &lval) && sval) printf("ziplist.e1=%.*s\n", (int)slen, sval);
    printf("ziplist.corrupt_valid=%d\n", ziplistValidateIntegrity(zl, ziplistBlobLen(zl) - 3, 1, NULL, NULL));
    zfree(zl);

    unsigned char *lp = lpNew(0);
    lp = lpAppend(lp, (unsigned char *)"abc", 3);
    lp = lpAppend(lp, (unsigned char *)"-77", 3);
    int64_t n; unsigned char ib[LP_INTBUF_SIZE];
    printf("listpack.len=%u\n", lpLength(lp));
    printf("listpack.valid=%d\n", lpValidateIntegrity(lp, lpBytes(lp), 1));
    unsigned char *e = lpFirst(lp);
    e = lpNext(lp, e);
    unsigned char *v = lpGet(e, &n, ib);
    if (v) printf("listpack.e1=str:%.*s\n", (int)n, v);
    else printf("listpack.e1=int:%lld\n", (long long)n);
    zfree(lp);

    intset *is = intsetNew();
    uint8_t ok;
    is = intsetAdd(is, 5, &ok);
    is = intsetAdd(is, -3, &ok);
    is = intsetAdd(is, 70000, &ok);
    is = intsetAdd(is, 5, &ok);
    printf("intset.len=%u dup_added=%d\n", intsetLen(is), ok);
    printf("intset.find70000=%d find9=%d\n", intsetFind(is, 70000), intsetFind(is, 9));
    int64_t first; intsetGet(is, 0, &first);
    printf("intset.first=%lld bytes=%zu\n", (long long)first, intsetBlobLen(is));
    printf("intset.valid=%d\n", intsetValidateIntegrity((unsigned char *)is, intsetBlobLen(is), 1));
    zfree(is);
    return 0;
}
