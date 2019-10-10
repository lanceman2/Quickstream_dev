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


/** A special value to mean an array of all channel numbers.
 *
 * The filter writer does not need to figure out how many channels there
 * are if they are just treating all the channels the same.  They can just
 * use this macro to mean all channels.
 *
 * This macro may be used in qsSetMaxReadThreshold(),
 * qsSetMinReadThreshold(), qsSetMaxReadSize(), and qsBufferCreate().
 */
#define QS_ALLCHANNELS   ((uint32_t *)(-1))


/** A special value or length to pass to qsOutput() to cause no output.
 */
#define QS_NONE          ((size_t) -1) // not 0




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
    QsFFinished = 1
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
 * \param inputChannelNum the channel designation for this input. A
 * transmitting (or feeder) filter is a filter that is outputting to a
 * receiver.  The receiver filter is the filter that has it's input()
 * function called.  For a given filter the input channels are numbered
 * from 0 to N-1 where N is the total number of input channels.  There may
 * be more than one input to a given receiver filter from a given
 * transmitter filter.
 *
 * \param flowState should be considered an opaque data type that is passed
 * in to the filter input() to let the filter know stuff about the state
 * of the stream flow.  inline functions provide ways to interpret this so
 * called state.  See \ref qsFlowIsLastPackage().
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
int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState);


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


/** get the default maximum length in bytes that may be written
 *
 * A filter that has output may set the maximum length in bytes that may
 * be written for a given qsOutput() call for a given output channel
 * number.  If the value of the maximum length in bytes that may be
 * written was not set in the filter start() function it's value will be
 * QS_DEFAULTWRITELENGTH.
 */
#define QS_DEFAULTWRITELENGTH  ((size_t) 1024)


/** get a buffer write pointer
 *
 * qsGetBuffer() may only be called in the filters input() function.  If a
 * given filter at a given \ref input() call will generate output than
 * qsGetBuffer() must be called.
 *
 * If output is to be generated for this output channel number than
 * qsOutput() must be called some time after this call to qsGetBuffer().
 *
 * If qsGetBuffer() is not called in a given filter input() callback
 * function there will be no output from the filter in the given input()
 * call.
 *
 * \param outputChannelNum the associated output channel.  If the output
 * channel is sharing a buffer between other output channels from this
 * filter then the value of outputChannelNum may be any of the output
 * channel numbers that are in the output channel shared buffer group.
 *
 * \return a pointer to the first writable byte in the buffer.
 */
extern
void *qsGetBuffer(uint32_t outputChannelNum);


/** trigger the call the listed output filters input() function
 *
 * qsGetBuffer() must be called before qsOutput().
 *
 * qsOutput() must be called in a filter module input() function in order
 * to have output to another filter module.
 *
 * \param len the length in bytes to advance the output buffer.  len may be
 * 0 to cause the corresponding output filters input() functions to be
 * called with an input length of 0.  Setting len to QS_NONE to stop the
 * corresponding output filters input() functions from being called.
 * Passing a len value of 0 will still trigger a call the listed output
 * filters input() function with an input length of 0, like it was a
 * source filter.
 *
 * \param outputChannelNum is the associated output channel.  If the buffer
 * of this output channel is shared between other output channels than
 * all the sharing output channels will also be used.
 */
extern
void qsOutput(size_t len, uint32_t outputChannelNum);


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
 * effects the current input channel number that passed to input();
 *
 * \param len advance the current input buffer len bytes.  len can be less
 * than or equal to the length in bytes that was passed to the input()
 * call.
 */
extern
void qsAdvanceInput(size_t len);


/** Set the maximum buffer read threshold
 *
 * qsSetMaxReadThreshold() may only be called in filters start() function.
 *
 * \param len This reading filter promises to read any data at or above
 * this threshold; so we will keep calling the filter input() function
 * until the amount that can be read is less than this threshold.
 *
 * \param inputChannelNums a list of effected input channel numbers as an
 * array of numbers.  Channel numbers start at 0 and run to N-1 where N is
 * the number of channels.
 */
extern
void qsSetMaxReadThreshold(size_t len, uint32_t *inputChannelNums);


/** Set the minimum buffer read threshold
 *
 * qsSetMinReadThreshold() may only be called in filters start() function.
 *
 * \param This reading filter will not read any data until this threshold,
 * len bytes, is met; so we will not call the filter input() function
 * until this threshold is met.
 *
 * \param inputChannelNums a list of effected input channel numbers as an
 * array of numbers.  Channel numbers start at 0 and run to N-1 where N is
 * the number of channels.
 */
extern
void qsSetMinReadThreshold(size_t len, uint32_t *inputChannelNums);


/** Set the maximum buffer read size passed to input()
 *
 * Filters may may request that an input channel not input past a set
 * amount.  This is not required to be set because an input channels
 * buffer advancement may be set to any amount that is less than of equal
 * to the amount sent in the call to the corresponding filters input()
 * call via the qsAdvanceInput() function.
 *
 * qsSetMaxReadSize() may only be called in filters start() function.
 *
 * \param len This reading filter will not read more than len bytes, if
 * this is set.  The filter sets this so that the stream running does not
 * call input() with more data than this.  This is a convenience, so the
 * filter does not need to tell the stream running to not advance the
 * buffer so far at every input() call had the input length exceeded this
 * number.
 *
 * \param inputChannelNums a list of effected input channel numbers as an
 * array of numbers.  Channel numbers start at 0 and run to N-1 where N is
 * the number of channels.
 */ 
extern
void qsSetMaxReadSize(size_t len, uint32_t *inputChannelNums);


#define QS_ARRAYTERM    ((uint32_t) -1)


/** Create an output buffer that is associated with the listed channels
 *
 * If there is more than one output channel number given than the ring
 * buffer will be shared between the output channels, and the data read by
 * all the receiving filters will be the same.
 *
 * qsBufferCreate() can only be called in the filter's start() function.
 * 
 * The total amount of memory allocated for this ring buffer depends on
 * maxWrite, and other parameters set by other filters that may be
 * accessing this buffer down stream.
 *
 * \param maxWriteLen this filter promises to write at most maxWriteLen
 * bytes to this output channel.  If the filter writes more than that
 * memory may be corrupted.
 *
 * \param outputChannelNums is a pointer to an array of output channel
 * numbers which is terminated with a value QS_ARRAYTERM.
 */
extern
void qsBufferCreate(size_t maxWriteLen, uint32_t *outputChannelNums);



/** Initialize simple help and argument parsing object
 *
 * A very simple command line option parser which parses "--" prefixed
 * command line options.  The --help option is built in and calls the
 * help() function that was set by the filter.
 *
 * \param opts a pointer to a stack or heap allocated struct QsOpts.
 * This memory is used for state in the object, but the user manages this
 * memory, and so there is no destructor function for the struct QsOpt.
 * struct QsOpt should be considered opaque by the user of this
 * function.
 *
 * argc and argv can and will likely be the values that have been passed
 * to the filter construct() function.
 *
 * \param argc the number of string arguments that argv points to.
 *
 * \param argv is the array of string arguments.
 */
extern
void qsOptsInit(struct QsOpts *opts, int argc, const char **argv);

/** Initialize simple help and argument parsing object
 *
 * \param opts should have been previously passed to qsOptInit().
 * \param optName is the name of the argument that was passed with the form
 * --name VAL.  optName should not start with "--", that gets added.
 *  \param defaultVal is the value that is returned if this argument option
 *  was no given in the filter arguments.
 *  \return the float value that the last option with this name was given
 *  in the command line, or defaultVal if this option was not given.
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



//In ../share/doc/quickstream/Doxyfile.in
// PREDEFINED = DOXYGEN_SHOULD_SKIP_THIS
//
// makes doxygen exclude documenting from here
//
#ifndef DOXYGEN_SHOULD_SKIP_THIS

// Unfortunately for performance we must expose these macros as user
// interfaces.  We are just avoiding dereferencing a pointer, but it will
// likely be happening at every call to input() at least in source
// filters.

// flow state bit flags
//
#  define _QS_LASTPACKAGE  01
//# define _QS_BLA_BLA_BLA  02


#endif // #ifndef DOXYGEN_SHOULD_SKIP_THIS  too here.


/** 
 * This function is only relevant when called in a filters input()
 * function.
 *
 *  /returns true if this is the last length of data to be input
 *  for a given input channel number.
 *
 *  \see input()
 */
static inline bool qsIsLastPackage(uint32_t state) {
    return _QS_LASTPACKAGE && state;
};




#ifdef __cplusplus
}
#endif

#endif // #ifndef __qsfilter_h__

