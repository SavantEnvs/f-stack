/* Known-answer probe: run nginx's request parser, URI normaliser and value parsers on fixed inputs. */
#include <stdio.h>
#include "../harnesses/nginx_env.h"

int main(void)
{
    mayhem_nginx_init();
    ngx_pool_t *pool;
    ngx_http_request_t *r = mayhem_new_request(&pool);
    char req[] = "GET /a%20b/../c?x=1 HTTP/1.1\r\nHost: example.com\r\nX-A: b c\r\n\r\n";
    ngx_buf_t b = {0};
    b.start = b.pos = (u_char *)req;
    b.last = b.end = (u_char *)req + sizeof(req) - 1;

    printf("nginx.request_line=%ld\n", (long)ngx_http_parse_request_line(r, &b));
    printf("nginx.method=%lu version=%lu\n", (unsigned long)r->method, (unsigned long)r->http_version);
    r->uri.len = r->args_start ? (size_t)(r->args_start - 1 - r->uri_start) : (size_t)(r->uri_end - r->uri_start);
    r->uri.data = ngx_pnalloc(pool, r->uri.len);
    ngx_http_parse_complex_uri(r, 1);
    printf("nginx.uri=%.*s\n", (int)r->uri.len, r->uri.data);
    printf("nginx.args=%.*s\n", (int)(r->uri_end - r->args_start), r->args_start);
    for (int i = 0; i < 3; i++) {
        ngx_int_t rc = ngx_http_parse_header_line(r, &b, 1);
        if (rc == NGX_OK)
            printf("nginx.header=%.*s:%.*s\n", (int)(r->header_name_end - r->header_name_start), r->header_name_start,
                   (int)(r->header_end - r->header_start), r->header_start);
        else
            printf("nginx.header_end=%ld\n", (long)rc);
    }

    u_char chunks[] = "5\r\nhello\r\n0\r\n\r\n";
    ngx_buf_t cb = {0};
    cb.start = cb.pos = chunks;
    cb.last = cb.end = chunks + sizeof(chunks) - 1;
    ngx_http_chunked_t ctx = {0};
    printf("nginx.chunk1=%ld size=%lld\n", (long)ngx_http_parse_chunked(r, &cb, &ctx, 0), (long long)ctx.size);
    printf("nginx.chunk2=%ld\n", (long)ngx_http_parse_chunked(r, &cb, &ctx, 0));

    u_char url[] = "10.0.0.1:8080";
    ngx_url_t u = {{0}};
    u.url.len = sizeof(url) - 1; u.url.data = url; u.default_port = 80; u.no_resolve = 1;
    printf("nginx.url=%ld port=%u host=%.*s\n", (long)ngx_parse_url(pool, &u), u.port, (int)u.host.len, u.host.data);
    printf("nginx.inet_addr=%lx\n", (unsigned long)ntohl(ngx_inet_addr((u_char *)"1.2.3.4", 7)));
    u_char date[] = "Sun, 06 Nov 1994 08:49:37 GMT";
    printf("nginx.http_time=%ld\n", (long)ngx_parse_http_time(date, sizeof(date) - 1));
    ngx_str_t sz = ngx_string("10k");
    printf("nginx.size=%ld\n", (long)ngx_parse_size(&sz));
    ngx_str_t tm = ngx_string("1h30m");
    printf("nginx.time=%ld\n", (long)ngx_parse_time(&tm, 1));
    return 0;
}
