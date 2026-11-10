/* The two hooks redisassert.h declares and src/debug.c defines (debug.c drags in the whole server). */
#include <stdio.h>
#include <stdlib.h>

void _serverAssert(const char *estr, const char *file, int line)
{
    fprintf(stderr, "=== ASSERTION FAILED ===\n==> %s:%d '%s' is not true\n", file, line, estr);
    abort();
}

void _serverPanic(const char *file, int line, const char *msg, ...)
{
    fprintf(stderr, "------------------------------------------------\n!!! Software Failure. Press left mouse button to continue\nGuru Meditation: %s #%s:%d\n", msg, file, line);
    abort();
}
