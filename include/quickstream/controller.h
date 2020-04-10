#ifndef __qscontroller_h__
#define __qscontroller_h__


/** \file
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
 * graph.  They access the filter Parameters and the quickstream \ref App
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
 * \todo A controller may also create and own parameters.
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


int preStart(struct QsStream *stream);
int postStart(struct QsStream *stream);

int preStop(struct QsStream *stream);
int postStop(struct QsStream *stream);

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


/** Register a pre-filter-input callback function
 *
 * When the controller or the associated filter are unloaded this callback
 * is removed.
 *
 * This must be called in one of the optional controller loaded functions:
 * construct(), preStart(), postStart(), preStop(), or postStop().
 *
 * /return 0 on success, and non-zero on failure.
 */
extern
int qsAddPreFilterInput(struct QsFilter *filter,
        const char *pName,
        int (*callback)(
            struct QsFilter *filter, const char *pName,
            const void *buffer[], const size_t len[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData), void *userData);


/** Register a post-filter-input callback function
 *
 * When the controller or the associated filter are unloaded this callback
 * is removed.
 *
 * This must be called in one of the optional controller loaded functions:
 * construct(), preStart(), postStart(), preStop(), or postStop().
 *
 * /return 0 on success, and non-zero on failure.
 */
extern
int qsAddPostFilterInput(struct QsFilter *filter,
        const char *pName,
        int (*callback)(
            struct QsFilter *filter, const char *pName,
            const void *buffer[], const size_t len[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData), void *userData);






#ifdef __cplusplus
}
#endif


#endif // #ifndef __qscontroller_h__
