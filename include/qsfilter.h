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



#define QS_ARRAYTERM    ((uint32_t) -1)


#define QS_ALLPORTS     ((uint32_t *) -2)


#define QS_NONE         ((size_t) -1)



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


#define QS_DEFAULT_MAXINPUT         ((size_t) 0) // not set


/** get the default maximum length in bytes that may be written
 *
 * A filter that has output may set the maximum length in bytes that may
 * be written for a given qsOutputs() and qsGetBuffer() calls for a given
 * output port number.  If the value of the maximum length in bytes that
 * may be written was not set in the filter start() function it's value
 * will be QS_DEFAULT_maxWrite.
 */
#define QS_DEFAULT_MAXWRITE         ((size_t) 1024)


/** \file
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
 * \param file output file to fprintf() to.
 *
 */
void help(FILE *file);



enum QsFilterInputReturn {
    QsFContinue = 0,
    QsFFinished = 1 // returned to start flushing
};


/** Required: filter input work function
 *
 * input() is how the filters receive input from upstream filters.
 *
 * \param buffer is a pointer to the current data being passed in from an
 * upstream filter, or 0 is this is a source filters.
 *
 * \param len the length of the data pasted in bytes.  len is 0 if this is
 * a source feed.
 *
 * \param inputPortNum the port designation for this input. A transmitting
 * (or feeder) filter is a filter that is outputting to a receiver.  The
 * receiver filter is the filter that has it's input() function called.
 * For a given filter the input ports are numbered from 0 to N-1 where N
 * is the total number of input ports.  There may be more than one input
 * to a given receiver filter from a given transmitter filter.
 *
 * \param flowState should be considered an opaque data type that is
 * passed in to the filter input() to let the filter know stuff about the
 * state of the stream flow.  inline functions provide ways to interpret
 * this so called state.  See \ref qsFlowIsLastPackage().
 *
 * \return The values returned from input() give the filters some control
 * over how the stream and it's flow behaves.  The return value 0 is the
 * most common return value telling the stream to continue flowing
 * normally.  
 *
 * \see qsFlowIsLastPackage().
 *
 * \todo figure out more return codes and what they mean
 */
int input(const void *buffer[], const size_t len[],
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
 * output to the ring buffer.  If the filter has a thread safe input()
 * function, that maxLen must be the same as the corresponding length
 * passed to qsOutput(), otherwise there will be unwritten gaps in the
 * ring buffer.
 *
 * \return a pointer to the first writable byte in the buffer.  The filter
 * may write at most \e maxLen bytes to this returned pointer.
 */
extern
void *qsGetBuffer(uint32_t outputPortNum, size_t maxLen);



/** advance data to the output buffers
 *
 * This stores the current buffer write offset by length len bytes.
 *
 * qsGetBuffer() must be called before qsOutputs().
 *
 * qsOutputs() must be called in a filter module input() function in order
 * to have output to another filter module.
 *
 * \param lens array of lengths in bytes to advance the output buffers.
 * length may be 0 to cause the corresponding output filters input()
 * functions to be called with an input length of 0.  Setting a length to
 * QS_NONE to stop the corresponding output filters input() functions from
 * being called.  Passing a len value of 0 will still trigger a call the
 * listed output filters input() function with an input length of 0, like
 * it was a source filter.
 */
extern
void qsOutputs(const size_t lens[]);



/** Advance the current buffer input len bytes
 *
 * qsAdvanceInputs() can only be called in a filter input() function.
 *
 * In order to advance the input buffer a length that is not the length
 * that was passed into the input() call, this qsAdvanceInputs() function
 * must be called in the input() function.  If qsAdvanceInputs() is not
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
void qsAdvanceInputs(const size_t lens[]);



/** Set the maximum buffer input read size passed to input()
 *
 * Filters may may request that an input port not input past a set
 * amount.  This is not required to be set because an input ports
 * buffer advancement may be set to any amount that is less than of equal
 * to the amount sent in the call to the corresponding filters input()
 * call via the accumulation of qsAdvanceInputs() function calls.
 *
 * qsSetInputMax() may only be called in filters start() function.
 *
 * \param len This reading filter will not read more than len bytes, if
 * this is set.  The filter sets this so that the stream running does not
 * call input() with more data than this.  This is a convenience, so the
 * filter does not need to tell the stream running to not advance the
 * buffer so far at every input() call had the input length exceeded this
 * number.
 */ 
extern
void qsSetInputMax(const size_t lens[]);



/** Create an output buffer that is associated with the listed ports
 *
 * If there is more than one output port number given than the ring
 * buffer will be shared between the output ports, and the data read by
 * all the receiving filters will be the same.
 *
 * qsBufferCreate() can only be called in the filter's start() function.
 * 
 * The total amount of memory allocated for this ring buffer depends on
 * maxWrite, and other parameters set by other filters that may be
 * accessing this buffer down stream.
 *
 * \param maxWriteLen this filter promises to write at most maxWriteLen
 * bytes to this output port.  If the filter writes more than that
 * memory may be corrupted.
 *
 * \param outputPortNums is a pointer to an array of output port numbers
 * which is terminated with a value QS_ARRAYTERM.
 */
extern
void qsBufferCreate(size_t maxWriteLen, const uint32_t outputPortNums[]);



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


#ifdef __cplusplus
}
#endif

#endif // #ifndef __qsfilter_h__
