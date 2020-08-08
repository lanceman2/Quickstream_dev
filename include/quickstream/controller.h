#ifndef __qscontroller_h__
#define __qscontroller_h__

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>


/** \page controllers controller modules
 *
 * \brief quickstream controller module writers application programming
 * interface (API) for C.
 *
 * This header file is to be included in C files that make controller
 * modules.
 *
 * Controllers are modules of code that are loaded via dynamic shared
 * objects (DSOs).  These module are the part of quickstream that monitors
 * and controls the quickstream flow.  They are not filters in the flow
 * graph.  They access the filter Parameters and the quickstream \ref app
 * API (application programming interface) to monitor and control the
 * stream flow.  The advantage of this Controller abstraction layer is
 * that filters need not know about the Controllers, and the Controllers
 * can do things like seamlessly provide a IoT (Internet of things)
 * interface to filters in the stream without the filter developers
 * knowing anything about it.  The filter only publish and serve
 * parameters though the quickstream \ref parameters API.  Simple put:
 * filters provide the knobs and the controllers turn and observe the
 * knobs.
 *
 * A controller may also create and own parameters.
 *
 * Because quickstream is breaking the rules and making interfaces that
 * are simplistic, users of the quckstream controllers API must take care
 * not to insert blocking calls in the filter I/O (input and output) calls
 * that would impend the stream flow.  The alternative is complexity,
 * piles of abstractions, tons of dependences, and tons of code (like
 * GNUradio).
 *
 * There are no required quickstream controller interface functions that a
 * controller module plugin must provide.
 *
 * The optional quickstream controller interfaces that a controller module
 * plugins may provide are: construct(), preStart(), postStart(),
 * preStop(), postStop(), destroy(), and help().  These functions are
 * called in quickstream applications at distinct times in the process of
 * running a quickstream stream flow graph.
 */

#ifndef __cplusplus


// TODO:
// For C++ code we define different versions of construct(), preStart(),
// postStart(), preStop(), postStop(), destroy(), and help(); that wrap
// the C++ filter module base QsController that is declared in
// controller.hpp.


struct QsFilter;
struct QsStream;


/** optional constructor function
 *
 * This function, if present, is called only once just after the
 * controller is loaded.
 *
 * \param argc the number of strings pointed to by argv
 *
 * \param argv an array of pointers to the string arguments.  The user
 * should not write to this memory.
 *
 * \return 0 on success, and non-zero on failure and then the loading of
 * the controller module is aborted.  If the value returned is less than 0
 * than an ERROR message will be set up.
 */
int construct(int argc, const char **argv);



/** print controller module help and command line argument options
 *
 * This function provides a brief description of what the controller does,
 * and maybe a URL to get more information about the controller, and a
 * list of command line argument options.  The output calling this
 * function should be the kind of thing produced from typical --help
 * command line option.
 *
 * When help() is called additional generic text will be added to the
 * output file that describes all quickstream controllers.
 *
 * \param file output FILE stream that may be called with the standard C
 * libraries fprintf() function.
 */
void help(FILE *file);


int preStart(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);
int postStart(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);

int preStop(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);
int postStop(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);


/** optional destructor function
 *
 * \return 0 on success and non-zero on failure, not that we do anything
 * about it anyway.
 */
int destroy(void);


#endif // ifndef __cplusplus


#ifdef __cplusplus
extern "C" {
#endif



/** Register a post-filter-input callback function
 *
 * When the controller or the associated filter are unloaded this callback
 * is removed.
 *
 * This must be called in one of the optional controller loaded functions:
 * construct(), preStart(), postStart(), preStop(), or postStop().
 *
 * Each controller may have only one post-input filter callback per filter.
 *
 * \param filter is the filter those input() function that is of concern.
 *
 * \param callback is the function that is called before each filter
 * input() is called.  \p callback() will be called with \p len that
 * reflects changes from the filter's input() call.  If a callback already
 * exists, the new callback will replace the old callback.
 *
 * If \p callback returns non-zero the callback will be removed.  The
 * len[] argument will be the set to the number of bytes that have been
 * pushed through the filter in the last filter input() call.
 *
 * If qsAddPostFilterInput() is called with \p callback zero the callback
 * will be removed.

 * \param userData is passed to the \p callback function each time it is
 * called.
 *
 * /return 0 on success.  Returns 1 if the keyName for this filter's
 * post-input callback already exists.
 *
 * /return 0 on success, and non-zero on failure.
 */
extern
int qsAddPostFilterInput(struct QsFilter *filter,
        int (*callback)(
            struct QsFilter *filter,
            const size_t lenIn[],
            const size_t lenOut[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData), void *userData);



#ifdef __cplusplus
}
#endif


#endif // #ifndef __qscontroller_h__
