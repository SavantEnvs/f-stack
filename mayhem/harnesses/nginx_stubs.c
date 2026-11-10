/* The globals and logging entry points nginx's core/os layers define in files that pull in the whole server
 * (ngx_cycle.c, ngx_log.c, ngx_process.c). The parsers only reach the log through ngx_log_error(), which
 * checks the level first, so a log that discards everything is equivalent here. */
#include <ngx_config.h>
#include <ngx_core.h>

volatile ngx_cycle_t  *ngx_cycle;
ngx_pid_t              ngx_pid;

void ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, ngx_err_t err, const char *fmt, ...)
{
    (void) level; (void) log; (void) err; (void) fmt;
}

void ngx_log_abort(ngx_err_t err, const char *fmt, ...)
{
    (void) err; (void) fmt;
    abort();
}
