#ifndef __debug_h__
#define __debug_h__


/** This file provides some CPP (C pre processor) macro debug functions:
 *
 *    ASSERT() FAIL()
 *
 * and compile time conditionally:
 *
 *    ERROR() WARN() NOTICE() INFO() DSPEW() DASSERT()
 *
 * see details below.
 *
 */

///////////////////////////////////////////////////////////////////////////
// USER DEFINABLE SELECTOR MACROS make the follow macros come to life:
//
// DEBUG             -->  DASSERT()
//
// SPEW_LEVEL_DEBUG  -->  DSPEW() INFO() NOTICE() WARN() ERROR()
// SPEW_LEVEL_INFO   -->  INFO() NOTICE() WARN() ERROR()
// SPEW_LEVEL_NOTICE -->  NOTICE() WARN() ERROR()
// SPEW_LEVEL_WARN   -->  WARN() ERROR()
// SPEW_LEVEL_ERROR  -->  ERROR()
//
// always on are     --> ASSERT() FAIL()
//
// If a macro function is not live it becomes a empty macro with no code.
//
//
// The ERROR() function will also set a string accessible through qsError()
//
//
///////////////////////////////////////////////////////////////////////////
//
// Setting SPEW_LEVEL_NONE with still have ASSERT() FAIL() spewing
// and ERROR() will not spew but will still set the qsError string
//


/*
 * It's really not much code but is a powerful development
 * tool like assert.  See 'man assert'.
 *
 * CRTSFilter builders do not have to include this file in their
 * plugin modules.  It's pretty handy for developing/debugging.
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif



#define SPEW_FILE stderr

///////////////////////////////////////////////////////////////////////////
//
// ##__VA_ARGS__ comma not eaten with -std=c++0x
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44317
//
// There is a GCC bug where GCC wrongly warns about ##__VA_ARGS__, so using
// #pragma GCC system_header suppresses these warnings.  Should this GCC
// bug get fixed, it'd be good to remove this next code line.
// See also: https://gcc.gnu.org/onlinedocs/cpp/System-Headers.html
#pragma GCC system_header

// We would like to be able to just call DSPEW() with no arguments
// which can make a zero length printf format.
#pragma GCC diagnostic ignored "-Wformat-zero-length"


extern void _spew(FILE *stream, int errn, const char *pre, const char *file,
        int line, const char *func, bool bufferIt, const char *fmt, ...)
         // check printf format errors at compile time:
        __attribute__ ((format (printf, 8, 9)));

extern void _assertAction(FILE *stream);



#  define _SPEW(stream, errn, bufferIt, pre, fmt, ... )\
     _spew(stream, errn, pre, __BASE_FILE__, __LINE__,\
        __func__, bufferIt, fmt "\n", ##__VA_ARGS__)

#  define ASSERT(val, ...) \
    do {\
        if(!((bool) (val))) {\
            _SPEW(SPEW_FILE, errno, true, "ASSERT("#val") failed: ", ##__VA_ARGS__);\
            _assertAction(SPEW_FILE);\
        }\
    }\
    while(0)

#  define FAIL(fmt, ...) \
    do {\
        _SPEW(SPEW_FILE, errno, true, "FAIL: ", fmt, ##__VA_ARGS__);\
        _assertAction(SPEW_FILE);\
    } while(0)


///////////////////////////////////////////////////////////////////////////
// We let the highest verbosity macro flag win.
//


#ifdef SPEW_LEVEL_DEBUG // The highest verbosity
#  ifndef SPEW_LEVEL_INFO
#    define SPEW_LEVEL_INFO
#  endif
#endif
#ifdef SPEW_LEVEL_INFO
#  ifndef SPEW_LEVEL_NOTICE
#    define SPEW_LEVEL_NOTICE
#  endif
#endif
#ifdef SPEW_LEVEL_NOTICE
#  ifndef SPEW_LEVEL_WARN
#    define SPEW_LEVEL_WARN
#  endif
#endif
#ifdef SPEW_LEVEL_WARN
#  ifndef SPEW_LEVEL_ERROR
#    define SPEW_LEVEL_ERROR
#  endif
#endif

#ifdef SPEW_LEVEL_ERROR
#  ifdef SPEW_LEVEL_NONE
#    undef SPEW_LEVEL_NONE
#  endif
#endif




#ifdef DEBUG
#  define DASSERT(val, fmt, ...) ASSERT(val, fmt, ##__VA_ARGS__)
#else
#  define DASSERT(val, fmt, ...) /*empty marco*/
#endif

#ifdef SPEW_LEVEL_NONE
#define ERROR(fmt, ...) _SPEW(0/*no spew stream*/, errno, true, "ERROR: ", fmt, ##__VA_ARGS__)
#else
#define ERROR(fmt, ...) _SPEW(SPEW_FILE, errno, true, "ERROR: ", fmt, ##__VA_ARGS__)
#endif

#ifdef SPEW_LEVEL_WARN
#  define WARN(fmt, ...) _SPEW(SPEW_FILE, errno, false, "WARN: ", fmt, ##__VA_ARGS__)
#else
#  define WARN(fmt, ...) /*empty macro*/
#endif 

#ifdef SPEW_LEVEL_NOTICE
#  define NOTICE(fmt, ...) _SPEW(SPEW_FILE, errno, false, "NOTICE: ", fmt, ##__VA_ARGS__)
#else
#  define NOTICE(fmt, ...) /*empty macro*/
#endif

#ifdef SPEW_LEVEL_INFO
#  define INFO(fmt, ...)   _SPEW(SPEW_FILE, 0, false, "INFO: ", fmt, ##__VA_ARGS__)
#else
#  define INFO(fmt, ...) /*empty macro*/
#endif

#ifdef SPEW_LEVEL_DEBUG
#  define DSPEW(fmt, ...)  _SPEW(SPEW_FILE, 0, false, "DEBUG: ", fmt, ##__VA_ARGS__)
#else
#  define DSPEW(fmt, ...) /*empty macro*/
#endif



#ifdef __cplusplus
}
#endif

#endif // #ifndef __debug_h__
