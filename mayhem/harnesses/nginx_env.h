/* Shared scaffolding for the nginx parser harnesses: the globals nginx's os/unix layer would define, a
 * request object backed by a real pool, and a log nginx's pool code can write to. */
#ifndef MAYHEM_NGINX_ENV_H
#define MAYHEM_NGINX_ENV_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_log_t mayhem_log;

static void mayhem_nginx_init(void)
{
    static int done;
    if (done) return;
    done = 1;
    ngx_pagesize = 4096;
    ngx_pagesize_shift = 12;
    ngx_cacheline_size = 64;
    mayhem_log.log_level = 0;
    ngx_cycle = NULL;
}

/* A request with nothing but a pool, as the parse functions need. */
static ngx_http_request_t *mayhem_new_request(ngx_pool_t **pool_out)
{
    ngx_pool_t *pool = ngx_create_pool(4096, &mayhem_log);
    if (!pool) return NULL;
    ngx_http_request_t *r = ngx_pcalloc(pool, sizeof(ngx_http_request_t));
    if (!r) { ngx_destroy_pool(pool); return NULL; }
    r->pool = pool;
    *pool_out = pool;
    return r;
}

#endif
