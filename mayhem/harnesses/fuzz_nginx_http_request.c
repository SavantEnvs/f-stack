/* nginx HTTP/1.x request parser (src/http/ngx_http_parse.c, the copy bundled in F-Stack's app/nginx-1.28.0):
 * request line, URI normalisation, then the header lines, in the order ngx_http_request.c drives them. */
#include "nginx_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mayhem_nginx_init();

    ngx_pool_t *pool;
    ngx_http_request_t *r = mayhem_new_request(&pool);
    if (!r) return -1;

    /* exact-size heap copy so ASan sees any read past the received bytes */
    u_char *copy = malloc(size ? size : 1);
    memcpy(copy, data, size);
    ngx_buf_t b;
    memset(&b, 0, sizeof(b));
    b.start = b.pos = copy;
    b.last = b.end = copy + size;

    int result = -1;
    if (ngx_http_parse_request_line(r, &b) == NGX_OK) {
        result = 0;

        /* ngx_http_process_request_uri() */
        if (r->args_start) {
            r->uri.len = r->args_start - 1 - r->uri_start;
        } else {
            r->uri.len = r->uri_end - r->uri_start;
        }
        if (r->complex_uri || r->quoted_uri || r->empty_path_in_uri) {
            if (r->empty_path_in_uri) r->uri.len++;
            r->uri.data = ngx_pnalloc(pool, r->uri.len);
            if (r->uri.data) ngx_http_parse_complex_uri(r, 1);
        } else {
            r->uri.data = r->uri_start;
        }

        /* ngx_http_process_request_headers() */
        for (;;) {
            ngx_int_t rc = ngx_http_parse_header_line(r, &b, 1);
            if (rc != NGX_OK) break;
        }
    }

    free(copy);
    ngx_destroy_pool(pool);
    return result;
}
