/* nginx URL / address / value parsers from src/core (ngx_inet.c, ngx_parse.c): ngx_parse_url (no DNS),
 * ngx_inet_addr, ngx_inet6_addr, ngx_parse_http_time, ngx_parse_size/time, as used for `listen`,
 * `proxy_pass`, `upstream server` and header values. */
#include "nginx_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mayhem_nginx_init();

    ngx_pool_t *pool = ngx_create_pool(4096, &mayhem_log);
    if (!pool) return -1;

    /* exact-size heap copy so ASan sees any read past the received bytes */
    u_char *copy = malloc(size ? size : 1);
    memcpy(copy, data, size);

    ngx_url_t u;
    memset(&u, 0, sizeof(u));
    u.url.len = size;
    u.url.data = copy;
    u.default_port = 80;
    u.no_resolve = 1;
    ngx_int_t url_rc = ngx_parse_url(pool, &u);

    ngx_inet_addr(copy, size);
#if (NGX_HAVE_INET6)
    u_char addr6[16];
    ngx_inet6_addr(copy, size, addr6);
#endif
    ngx_parse_http_time(copy, size);
    ngx_str_t s = { size, copy };
    ngx_parse_size(&s);
    ngx_parse_offset(&s);
    ngx_parse_time(&s, 0);
    ngx_parse_time(&s, 1);

    ngx_destroy_pool(pool);
    free(copy);
    return url_rc == NGX_OK ? 0 : -1;
}
