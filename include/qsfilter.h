#ifndef __qsfilter_h__
#define __qsfilter_h__

#include <inttypes.h>
#include <stdio.h>
///
// quickstream filter interfaces that the user may provide in their
// filter module.
//


#define QS_ALLCHANNELS   ((uint32_t) -1)



/** \file
 *
 * The required quickstream filter interfaces that the filter module
 * plugins may provide are: input() and help().
 *
 * The optional quickstream filter interfaces that the filter module
 * plugins may provide are: construct(), destroy(), start(), and stop().
 *
 * The libquickstream library provides utility functions that the filter
 * module plugins may use.
 *
 */


#ifdef __cplusplus
extern "C" {
#endif


struct QsOpts; 


/** Required: print filter module help and command line argument options
 *
 * This function provides a brief description of what the filter
 * does, a URL to get more information about the filter, and
 * a list of command line argument options.  The kind of thing produced
 * from the --help command line option.
 *
 * When help() is called additional generic text will be added to the
 * output file that describes all quickstream filters.
 *
 * /para file output file to fprintf() to.
 *
 */
void help(FILE *file);


/** Required: filter input work function
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int input(void *buffer, size_t len, uint32_t inputChannelNum);


/** Optional constructor function
 *
 * This function, if present, is called only once just after the filter
 * is loaded.
 *
 * \para argc the number of strings pointed to by argv
 *
 * \para argv an array of pointers to the string arguments.  The user
 * should not write to this memory.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int constructor(int argc, const char **argv);


/** Optional destructor function
 *
 * This function, if present, is called only once just before the filter
 * is unloaded.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int destructor(void);


/** Optional start function
 *
 * This function, if present, is called each time the stream starts
 * running, just before any filter in the stream has it's \ref input()
 * function called.  After this is called \ref input() will be called
 * in a regular fashion.
 *
 * This function lets that filter determine what the number of inputs and
 * outputs will be before it has it's \ref input() function called.  For
 * "smarter" filters this can spawn a series of initialization interactions
 * between the "smarter" filters in the stream.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int start(uint32_t numInChannels, uint32_t numOutChannels);


/** Optional start function
 *
 * This function, if present, is called each time the stream starts
 * running, just before any filter in the stream has it's \ref input()
 * function called.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int stop(uint32_t numInChannels, uint32_t numOutChannels);



extern
void *qsBufferGet(size_t len, uint32_t outputChannelNum);

extern
void qsPush(size_t len, uint32_t outputChannelNum);



/** Initialize simple help and argument parsing object
 *
 * \para opts a pointer to a stack or heap allocated struct QsOpts.
 * This memory is used for state in the object, but the user manages this
 * memory, and so there is no destructor function for the struct QsOpt.
 * struct QsOpt should be considered opaque by the user of this
 * function.
 *
 * argc and argv can and will likely be the values that have been passed
 * to the filter construct() function.
 *
 * \para argc the number of string arguments that argv points to.
 *
 * \para argv is the array of string arguments.
 */
extern
void qsOptsInit(struct QsOpts *opts, int argc, const char **argv);

/** Initialize simple help and argument parsing object
 *
 * \para opts should have been previously passed to qsOptInit().
 */
extern
float qsOptsGetFloat(struct QsOpts *opts, const char *optName,
        float defaultVal);

extern
double qsOptsGetDouble(struct QsOpts *opts, const char *optName,
        double defaultVal);

extern
const char *qsOptsGetString(struct QsOpts *opts, const char *optName,
        const char *defaultVal);

extern
int qsOptsGetInt(struct QsOpts *opts, const char *optName,
        int defaultVal);



#ifdef __cplusplus
}
#endif

#endif // #ifndef __qsfilter_h__

