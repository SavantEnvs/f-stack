/* nginx chunked-body decoder (ngx_http_parse_chunked in src/http/ngx_http_parse.c), fed the way
 * ngx_http_request_body.c feeds it: repeatedly over one buffer until it stops asking for more or finishes. */
#include "nginx_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mayhem_nginx_init();

    ngx_pool_t *pool;
    ngx_http_request_t *r = mayhem_new_request(&pool);
    if (!r) return -1;

    u_char *copy = malloc(size ? size : 1);
    memcpy(copy, data, size);
    ngx_buf_t b;
    memset(&b, 0, sizeof(b));
    b.start = b.pos = copy;
    b.last = b.end = copy + size;

    ngx_http_chunked_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    ngx_int_t rc;
    int result = 0;
    do {
        rc = ngx_http_parse_chunked(r, &b, &ctx, 1);
        if (rc == NGX_ERROR) result = -1;
        /* more bytes are only needed on AGAIN; a buffer with nothing left cannot make progress */
    } while (rc == NGX_AGAIN && b.pos < b.last);

    free(copy);
    ngx_destroy_pool(pool);
    return result;
}
