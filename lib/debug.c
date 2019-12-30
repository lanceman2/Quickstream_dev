#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <pthread.h>

#ifndef SYS_gettid
#  error "SYS_gettid unavailable on this system"
#endif

#include "./debug.h"
#include "../include/qsapp.h"


// We make the access to qsErrorBuffer thread safe:
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static char *qsErrorBuffer = 0;

#define BUFLEN  1024

//
// The non-zero return value must be free()ed by the API user.
//
// API interface.  Thread safe.
//
const char *qsError(void) {

    // Adds a pointer for every thread created; is better than adding the
    // whole string of memory.
    static __thread char *buffer = 0;

    if(buffer) {
        // cleanup from the last call from this thread.
        free(buffer);
        buffer = 0;
    }

    // If this pthread_mutex_lock() fails we can't do much about it
    // because we are in the code that is our error handling code.
    pthread_mutex_lock(&mutex);
    if(qsErrorBuffer) {
        // copy the error string
        buffer = strdup(qsErrorBuffer);
        // clear the error string
        qsErrorBuffer = 0;
    }
    pthread_mutex_unlock(&mutex);
    // return the copy if there was one.
    return (const char *) buffer;
}


static void _vspew(FILE *stream, int errn, const char *pre, const char *file,
        int line, const char *func, bool bufferIt,
        const char *fmt, va_list ap)
{
    // We try to buffer this so that prints do not get intermixed with
    // other prints in multi-threaded programs.
    char buffer[BUFLEN];
    int len;

    if(errn)
    {
        char estr[128];
        strerror_r(errn, estr, 128);
        len = snprintf(buffer, BUFLEN, "%s%s:%d:pid=%u:%zu %s():errno=%d:%s: ",
                pre, file, line,
                getpid(), syscall(SYS_gettid), func,
                errn, estr);
    }
    else
        len = snprintf(buffer, BUFLEN, "%s%s:%d:pid=%u:%zu %s(): ", pre, file, line,
                getpid(), syscall(SYS_gettid), func);

    if(len < 10 || len > BUFLEN - 40)
    {
        //
        // This should not happen.

        //
        // Try again without stack buffering
        //
        if(stream) {
            if(errn) {
                char estr[128];
                strerror_r(errn, estr, 128);
                fprintf(stream, "%s%s:%d:pid=%u:%zu %s():errno=%d:%s: ",
                    pre, file, line,
                    getpid(), syscall(SYS_gettid), func,
                    errn, estr);
            } else
                fprintf(stream, "%s%s:%d:pid=%u:%zu %s(): ", pre, file, line,
                        getpid(), syscall(SYS_gettid), func);
        }

        int ret;
        ret = vsnprintf(&buffer[len], BUFLEN - len,  fmt, ap);
        if(ret > 0) len += ret;
        if(len + 1 < BUFLEN) {
            // Add newline to the end.
            buffer[len] = '\n';
            buffer[len+1] = '\0';
        }

        return;
    }

    int ret;
    ret = vsnprintf(&buffer[len], BUFLEN - len,  fmt, ap);
    if(ret > 0) len += ret;
    if(len + 1 < BUFLEN) {
        // Add newline to the end.
        buffer[len] = '\n';
        buffer[len+1] = '\0';
    }

    if(stream)
        fputs(buffer, stream);

    if(bufferIt) {
        //
        // Put the error string into a globally accessible qsError()
        // buffer.
        //

        // If this pthread_mutex_lock() fails we can't do much about it
        // because we are in the code that is our error handling code.
        pthread_mutex_lock(&mutex);
        qsErrorBuffer = realloc(qsErrorBuffer, strlen(buffer) + 1);
        strcpy(qsErrorBuffer, buffer);
        pthread_mutex_unlock(&mutex);
    }
}


void qs_spew(FILE *stream, int errn, const char *pre, const char *file,
        int line, const char *func, bool bufferIt, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _vspew(stream, errn, pre, file, line, func, bufferIt, fmt, ap);
    va_end(ap);
}


void qs_assertAction(FILE *stream)
{
    pid_t pid;
    pid = getpid();
#ifdef ASSERT_ACTION_EXIT
    fprintf(stream, "Will exit due to error\n");
    exit(1); // atexit() calls are called
    // See `man 3 exit' and `man _exit'
#else // ASSERT_ACTION_SLEEP
    int i = 1; // User debugger controller, unset to effect running code.
    fprintf(stream, "  Consider running: \n\n  gdb -pid %u\n\n  "
        "pid=%u:%zu will now SLEEP ...\n", pid, pid, syscall(SYS_gettid));
    while(i) { sleep(1); }
#endif
}
