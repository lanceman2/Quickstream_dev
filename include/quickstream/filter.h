#ifndef __qsfilter_h__
#define __qsfilter_h__


#include <stdio.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
///
// quickstream filter interfaces that the user may provide in their
// filter module.
//




// Given the parameters in these data structures (QsOutput and QsBuffer)
// the stream running code should be able to determine the size needed for
// the associated circular buffers.
//
// QsOutput is the data for a reader filter that another filter feeds
// data.
//
// We cannot put all the ring buffer data in one data structure because
// the ring buffer can be configured/shared between filters in different
// ways.



/** The default input read promise
 */
#define QS_DEFAULTMAXREADPROMISE    ((size_t) 1024)


/** The default maximum length in bytes that may be written
 *
 * A filter that has output may set the maximum length in bytes that may
 * be written for a given qsOutput() and qsGetBuffer() calls for a given
 * output port number.  If the value of the maximum length in bytes that
 * may be written was not set in the filter start() function it's value
 * will be QS_DEFAULTMAXWRITE.
 */
#define QS_DEFAULTMAXWRITE         ((size_t) 1024)


/** In general the idea of threshold is related to input triggering.
 * In the simplest case we can set a input channel threshold.
 */
#define QS_DEFAULTTHRESHOLD        ((size_t) 1)





/** \file quickstream 
 *
 * \brief filter module writers application programming interface (API)
 *
 * The required quickstream filter interfaces that the filter module
 * plugins may provide are: input() and help().
 *
 * The optional quickstream filter interfaces that the filter module
 * plugins may provide are: construct(), destroy(), start(), and stop().
 *
 * The libquickstream library provides utility functions that the filter
 * module plugins may use are declared in this header file.
 *
 * Lock-less shared ring buffers are used to pass data between filter
 * input() (or work) functions.  The size of the ring buffers depends on
 * parameters set in both the sending and the receiving filters.  These
 * buffer parameters may optionally be set in the filter start()
 * functions.
 */


#ifdef __cplusplus
extern "C" {
#endif



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
 * \param file output file to fprintf() to.
 *
 */
void help(FILE *file);




/** Required: filter input work function
 *
 * input() is how the filters receive input from upstream filters.
 *
 * \param inBuffers is a pointer to an array of the current data being
 * passed in from an upstream filter, or 0 is this is a source filters.
 * inBuffers is not const in case the filter uses the buffer to pass
 * through data to a down stream filter by making the ports buffer a "pass
 * through" buffer.
 *
 * \param inLens is a pointer to an array of the size of the data in the
 * inBuffers array in bytes.  inLens is 0 if this is a source feed that
 * has no inputs.
 *
 * \param sFlushing is a 
 *
 * \return The values returned from input() give the filters some control
 * over how the stream and it's flow behaves.  The return value 0 is the
 * most common return value telling the stream to continue flowing
 * normally.  Returning a any non-zero value stops that calling of
 * input() and sets up the final flushing of the down stream filters.  A
 * return value that is negative is an considered an "error like" case and
 * will cause a warning to be printed to stderr.  A positive return will
 * not do that.
 *
 * \todo figure out more return codes and what they mean
 */
int input(void *inBuffers[], const size_t inLens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts);


/** Optional constructor function
 *
 * This function, if present, is called only once just after the filter
 * is loaded.
 *
 * \param argc the number of strings pointed to by argv
 *
 * \param argv an array of pointers to the string arguments.  The user
 * should not write to this memory.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int construct(int argc, const char **argv);


/** Optional destructor function
 *
 * This function, if present, is called only once just before the filter
 * is unloaded.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int destroy(void);


/** Optional filter start function
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
int start(uint32_t numInPorts, uint32_t numOutPorts);


/** Optional filter stop function
 *
 * This function, if present, is called each time the stream stops
 * running, just after any filter in the stream has it's \ref input()
 * function called for the last time in a flow/run cycle.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int stop(uint32_t numInPorts, uint32_t numOutPorts);


/** get a output buffer pointer
 *
 * qsGetBuffer() may only be called in the filters input() function.  If a
 * given filter at a given \ref input() call will generate output than
 * qsGetBuffer() must be called and later followed by a call to
 * qsOutput().
 *
 * If output is to be generated for this output port number than
 * qsOutput() must be called some time after this call to qsGetBuffer().
 *
 * If qsGetBuffer() is not called in a given filter input() callback
 * function there will be no output from the filter in the given input()
 * call.
 *
 * \param outputPortNum the associated output port.  If the output
 * port is sharing a buffer between other output ports from this
 * filter then the value of outputPortNum may be any of the output
 * port numbers that are in the output port shared buffer group.
 *
 * \param maxLen is the maximum length in bytes that the filter will
 * output to the ring buffer.
 *
 * \param minLen is the minimum length in bytes that the filter will
 * output to the ring buffer.
 *
 * If the filter is thread-safe and maxLen equals minLen, than the ring
 * buffer may be accessed by another thread calling qsGetBuffer(),
 * otherwise if maxLen and minLen are not equal than another thread may
 * not access the ring buffer until qsOutput() is called in the
 * corresponding thread.
 *
 * \return a pointer to the first writable byte in the buffer.  The filter
 * may write at most \e maxLen bytes to this returned pointer.
 */
extern
void *qsGetOutputBuffer(uint32_t outputPortNum,
        size_t maxLen, size_t minLen);



/** advance data to the output buffers
 *
 * This stores the current buffer write offset by length len bytes.
 *
 * qsGetBuffer() must be called before qsOutput().
 *
 * qsOutput() must be called in a filter module input() function in order
 * to have output to another filter module.
 *
 * \param len length in bytes to advance the output buffer.
 */
extern
void qsOutput(uint32_t outputPortNum, size_t len);



/** Advance the current buffer input len bytes
 *
 * qsAdvanceInput() can only be called in a filter input() function.
 *
 * In order to advance the input buffer a length that is not the length
 * that was passed into the input() call, this qsAdvanceInput() function
 * must be called in the input() function.  If qsAdvanceInput() is not
 * called in input() than the input buffer will automatically be advanced
 * the length that was passed to input().
 *
 * This has no effect on output from the current filter.  This only
 * effects the current input port number that passed to input();
 *
 * \param lens advance the current input buffer lens bytes.  len can be
 * less than or equal to the corresponding length in bytes that was passed
 * to the input() call.  len greater than the input length will be
 * clipped.
 */
extern
void qsAdvanceInput(uint32_t inputPortNum, size_t len);



extern
void qsSetInputThreshold(uint32_t inputPortNum, size_t len);



// Sets maxRead
/** Set the input read promise
 *
 * The filter input() promises to read at least one byte of data on a
 * given input() call, if there is len bytes of input on the port
 * inputPortNum to read.  If this promise is not kept the program will
 * fail.  This is to keep the fixed ring buffers from being overrun.
 *
 * \param inputPortNum the input port number that the promise is made for.
 *
 * \param len length in bytes.
 *
 * The default len value is QS_DEFAULTMAXREADPROMISE.  If this default
 * value is not large enough than you must call this.
 */
extern
void qsSetInputReadPromise(uint32_t inputPortNum, size_t len);





/** Create an output buffer that is associated with the listed ports
 *
 * qsOutputBufferCreate() can only be called in the filter's start()
 * function.  If qsOutputBufferCreate() is not called for a given port
 * number an output buffer will be created with the maxWrite parameter set
 * to the default value of QS_DEFAULTMAXWRITE.
 * 
 * The total amount of memory allocated for this ring buffer depends on
 * maxWrite, and other parameters set by other filters that may be
 * accessing this buffer down stream.
 *
 * \param outputPortNum the output port number that the filter will use to
 * write to this buffer via qsGetBuffer() and qsOutput().
 *
 * \param maxWriteLen this filter promises to write at most maxWriteLen
 * bytes to this output port.  If the filter writes more than that memory
 * may be corrupted.
 */
extern
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen);



extern
int qsCreatePassThroughBuffer(uint32_t inPortNum, uint32_t outputPortNum,
        size_t maxWriteLen);


// Because "C" does not know what a pointer to a struct is in
// qsCreatePassThroughBufferDownstream() below.
struct QsFilter;

/** Make a downstream filter read from a pass through buffer.
 *
 * \return 0 on success, or -1 if there is already another downstream
 * filter that is using this output port as a pass through buffer. 
 */
extern
int qsCreatePassThroughBufferDownstream(uint32_t outputPortNum,
        struct QsFilter *toFilter, uint32_t toInputPort);



/** Tell quickstream that the filters input() function is thread safe.
 *
 * This must be called in the filter construct() function.
 *
 * This will let more than one thread call the filter input() function at
 * a time.
 *
 * \param maxThreads the maximum number of thread that will call the
 * filters input() function.
 *
 * By default, quickstream filter input() functions are assumed to not be
 * thread safe, so, by default, there will only be one thread calling a
 * filter input() function at a time.
 */
extern
void qsSetThreadSafe(uint32_t maxThreads);


/** Get a float as an option argument.
 *
 * \param argc is the length of the argv array of strings.
 * \param argv is a pointer to an array of strings that are the arguments.
 * \param optName is the name of the argument that was passed with the
 * form --name VAL or --name=VAL.  optName should not start with "--",
 * that gets added.
 * \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 * \return the float value that the last option with this name was given
 *  in the command line, or defaultVal if this option was not given.
 */
extern
float qsOptsGetFloat(int argc, const char **argv,
        const char *optName, float defaultVal);


/** Get a double as an option argument.
 *
 * \param argc is the length of the argv array of strings.
 * \param argv is a pointer to an array of strings that are the arguments.
 * \param optName is the name of the argument that was passed with the
 * form --name VAL or --name=VAL.  optName should not start with "--",
 * that gets added.
 * \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 * \return the double value that the last option with this name was given
 *  in the command line, or defaultVal if this option was not given.
 */
extern
double qsOptsGetDouble(int argc, const char **argv,
        const char *optName, double defaultVal);


/** Get a string as an option argument.
 *
 * \param argc is the length of the argv array of strings.
 * \param argv is a pointer to an array of strings that are the arguments.
 * \param optName is the name of the argument that was passed with the
 * form --name VAL or --name=VAL.  optName should not start with "--",
 * that gets added.
 * \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 * \return pointer to the string last option with this name was given
 *  in the command line, or defaultVal if this option was not given.
 */
extern
const char *qsOptsGetString(int argc, const char **argv,
        const char *optName, const char *defaultVal);


/** Get an int as an option argument.
 *
 * \param argc is the length of the argv array of strings.
 * \param argv is a pointer to an array of strings that are the arguments.
 * \param optName is the name of the argument that was passed with the
 * form --name VAL or --name=VAL.  optName should not start with "--",
 * that gets added.
 * \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 * \return an int last option with this name was given in the command
 * line, or defaultVal if this option was not given.
 */
extern
int qsOptsGetInt(int argc, const char **argv,
        const char *optName, int defaultVal);


/** Get an size_t as an option argument.
 *
 * \param argc is the length of the argv array of strings.
 * \param argv is a pointer to an array of strings that are the arguments.
 * \param optName is the name of the argument that was passed with the
 * form --name VAL or --name=VAL.  optName should not start with "--",
 * that gets added.
 * \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 * \return a size_t last option with this name was given in the command
 * line, or defaultVal if this option was not given.
 */
extern
size_t qsOptsGetSizeT(int argc, const char **argv,
        const char *optName, size_t defaultVal);


/** Get an uint32_t as an option argument.
 *
 * \param argc is the length of the argv array of strings.
 * \param argv is a pointer to an array of strings that are the arguments.
 * \param optName is the name of the argument that was passed with the
 * form --name VAL or --name=VAL.  optName should not start with "--",
 * that gets added.
 * \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 * \return an uint32_t last option with this name was given in the command
 * line, or defaultVal if this option was not given.
 */
extern
int32_t qsOptsGetUint32(int argc, const char **argv, const char *optName,
        uint32_t defaultVal);


extern
void qsRemoveDefaultFilterOptions(int argc, const char **argv);


#ifdef __cplusplus
}
#endif

#endif // #ifndef __qsfilter_h__
