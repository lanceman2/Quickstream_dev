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





/** \file
 *
 * \brief filter module writers application programming interface (API)
 * for C.
 *
 * The one required quickstream filter interface that a filter module
 * plugin must provide is input().
 *
 * The optional quickstream filter interfaces that a filter module
 * plugins may provide are: construct(), destroy(), start(), stop(),
 * and help().
 *
 * The libquickstream library provides utility functions that the filter
 * module plugins may use are declared in this header file \ref filter.h
 *
 * Lock-less shared ring buffers are used to pass data between filter
 * input() (or work) functions.  The size of the ring buffers depends on
 * parameters set in both the sending and the receiving filters.  These
 * buffer parameters may optionally be set in the filter start()
 * functions.
 */


#ifndef __cplusplus

// For C++ code we define different versions of construct(), input(),
// start(), stop(), destroy, and help(); that wrap the C++ filter module
// base QsFilter that is declared in filter.hpp.


/**
 * \headerfile filter.h "quickstream/filter.h"
 */

/** \class CFilterAPI filter.h "quickstream/filter.h"
 */


/** print filter module help and command line argument options
 *
 * This function provides a brief description of what the filter does, and
 * maybe a URL to get more information about the filter, and a list of
 * command line argument options.  The output calling this function should
 * be the kind of thing produced from typical --help command line option.
 *
 * When help() is called additional generic text will be added to the
 * output file that describes all quickstream filters.
 *
 * \param file output FILE stream that may be called with the standard C
 * libraries fprintf() function.
 */
void help(FILE *file);


/** required filter input work function
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
 * \param isFlushing is a array of bools that show which input ports are
 * being flushed from up stream.  When a input port is being flushed than
 * the input data in the buffer for that port will not be added to until
 * the next stream flow cycle.
 *
 * \param numInPorts is the number of input buffers in the inBuffers input
 * array.  numInPorts will be the same value for the duration of the
 * current flow cycle.
 *
 * \param numOutPorts is the number of output buffers that this filter is
 * writing to.  numOutPorts will be the same value for the duration of the
 * current flow cycle.
 *
 * \return the values returned from input() give the filters some control
 * over how the stream and it's flow behaves.  The return value 0 is the
 * most common return value telling the stream to continue flowing
 * normally.  Returning a any non-zero value stops the calling of
 * input() for the current filter and sets up the final flushing of the
 * down stream filters this the current filter is feeding.  A return value
 * that is negative is considered an "error like" case and will cause a
 * warning to be printed to stderr.  A positive return will not do that.
 */
int input(void *inBuffers[], const size_t inLens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts);


/** optional constructor function
 *
 * This function, if present, is called only once just after the filter
 * is loaded.
 *
 * \param argc the number of strings pointed to by argv
 *
 * \param argv an array of pointers to the string arguments.  The user
 * should not write to this memory.
 *
 * \return 0 on success, and non-zero on failure.
 */
int construct(int argc, const char **argv);


/** optional destructor function
 *
 * This function, if present, is called only once just before the filter
 * is unloaded.
 *
 * \return 0 on success, and non-zero on failure.
 */
int destroy(void);


/** optional filter start function
 *
 * This function, if present, is called each time the stream starts
 * running, just before any filter in the stream has it's \ref input()
 * function called.  We call this time the start of a flow cycle.  After
 * this is called \ref input() will be called in a regular fashion for
 * the duration of the flow cycle.
 *
 * This function lets that filter determine what the number of inputs and
 * outputs before it has it's \ref input() function called.  For "smarter"
 * filters this can spawn a series of initialization interactions between
 * the "smarter" filters in the stream.
 *
 * The following functions may only be called in the filters start()
 * function: qsCreateOutputBuffer(), qsCreatePassThroughBuffer(),
 * qsSetInputThreshold(), qsSetInputReadPromise(), and
 * qsGetFilterName().
 *
 * \param numInPorts is the number of input buffers in the inBuffers input
 * array.  numInPorts will be the same value for the duration of the
 * current flow cycle.
 *
 * \param numOutPorts is the number of output buffers that this filter is
 * writing to.  numOutPorts will be the same value for the duration of the
 * current flow cycle.
 *
 * \return 0 on success, or non-zero on failure.
 */
int start(uint32_t numInPorts, uint32_t numOutPorts);


/** Optional filter stop function
 *
 * This function, if present, is called each time the stream stops
 * running, just after any filter in the stream has it's \ref input()
 * function called for the last time in a flow/run cycle.
 *
 * \param numInPorts is the number of input buffers was in inBuffers input
 * array.  numInPorts will be the same value as 173it was when the
 * corresponding start() and input() was called.
 *
 * \param numOutPorts is the number of output buffers that this filter is
 * writing to.  numOutPorts will be the same value as it was when the
 * corresponding start() and input() was called.
 *
 * \return 0 on success, or non-zero on failure.
 */
int stop(uint32_t numInPorts, uint32_t numOutPorts);


#endif // ifndef __cplusplus


#ifdef __cplusplus
extern "C" {
#endif



/** get a output buffer pointer
 *
 * qsGetBuffer() may only be called in the filters input() function.  If a
 * given filter at a given input() call will generate output than
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
 * corresponding thread.  See qsSetThreadSafe().
 *
 * \return a pointer to the first writable byte in the buffer.  The filter
 * may write at most \e maxLen bytes to this returned pointer.
 */
extern
void *qsGetOutputBuffer(uint32_t outputPortNum,
        size_t maxLen, size_t minLen);


/** advance data to the output buffers
 *
 * This set the current buffer write offset by advancing it \p len bytes.
 *
 * qsGetOutputBuffer() should be called before qsOutput().
 *
 * qsOutput() must be called in a filter module input() function in order
 * to have output sent to another filter module.
 *
 * \param outputPortNum the output port number.
 *
 * \param len length in bytes to advance the output buffer.
 */
extern
void qsOutput(uint32_t outputPortNum, size_t len);


/** advance the current input buffer
 *
 * qsAdvanceInput() can only be called in a filter input() function.
 *
 * In order to advance the input buffer a length that is not the length
 * that was passed into the input() call, this qsAdvanceInput() function
 * must be called in the input() function.  If qsAdvanceInput() is not
 * called in input() than the input buffer will not be automatically be
 * advanced, and an under-run error can occur is the read promise for that
 * port is not fulfilled.  See qsSetInputReadPromise().
 *
 * This has no effect on output from the current filter.  This only
 * effects the current input port number that passed to input();
 *
 * \param inputPortNum the input port number that corresponds with the
 * input buffer pointer that you wish to mark as read.
 *
 * \param len advance the current input buffer length in bytes.  \p len
 * can be less than or equal to the corresponding length in bytes that was
 * passed to the input() call.  len greater than the input length will be
 * clipped to the length that was passed to input().
 */
extern
void qsAdvanceInput(uint32_t inputPortNum, size_t len);


/** set the current filters input threshold
 *
 * Set the minimum input needed in order for current filters input()
 * function to be called.  This threshold, if reached for the
 * corresponding input port number, will cause input() to be called.  If
 * this simple threshold condition is not adequate the filters input()
 * function may quickly just return 0, and effectively wait for more a
 * complex threshold condition to be reached by continuing to just quickly
 * return 0 until the filter sees the level of inputs that it likes.
 *
 * qsSetInputThreshold() may only be called in the filters start()
 * function.
 *
 * \param inputPortNum the input port number that corresponds with the
 * input buffer pointer that you wish to set a simple threshold for.
 *
 * \param len the length in bytes of this simple threshold.
 */
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
 * \param len length in bytes that the current filter promises to read.
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
 * write to this buffer via qsGetOutputBuffer() and qsOutput().
 *
 * \param maxWriteLen this filter promises to write at most maxWriteLen
 * bytes to this output port.  If the filter writes more than that memory
 * may be corrupted.
 */
extern
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen);

/** create a "pass-through" buffer
 *
 * A pass-through buffer shared the memory mapping between the input port
 * to an output port.  The filter reading the input can write it's output
 * to the same virtual address as the input.  The input is overwritten by
 * the output.  The size of the input data must be the same as the output
 * data, given they are the same memory.  We are just changing the value
 * in the input memory and passing it through to the output; whereby
 * saving the need for transferring data between two separate memory
 * buffers.
 *
 * \param inputPortNum the input port number that will share memory with
 * the output.
 *
 * \param outputPortNum the output port number that will share memory with
 * the input.
 *
 * \param maxWriteLen is the maximum length in bytes that the calling
 * filter promises not to exceed calling qsOutput().
 *
 * \return 0 on success.  If the buffer that corresponds with the output port
 * is already passed through to this or another input to any filter this
 * with fail and return non-zero.
 */
extern
int qsCreatePassThroughBuffer(uint32_t inputPortNum, uint32_t outputPortNum,
        size_t maxWriteLen);


// Because "C" does not know what a pointer to a struct is in
// qsCreatePassThroughBufferDownstream() below.
struct QsFilter;


/** Get the name of the filter
 *
 * The name of a filter is set by the person running the quickstream
 * program.  The name is unique for that loaded filter running in a given
 * program.  The name is generated automatically of can be set using the
 * qsAppFilterLoad() function in a quickstream runner program.
 *
 * qsGetFilterName() can only be called in a filter module in it's
 * construct(), start(), stop(), and destroy() functions.
 * qsGetFilterName() is not available in the filter's help() function.
 * Calling qsGetFilterName() in the filter's help() makes no sense given
 * that help() is not called in a stream flow time scenario when the
 * filter's name is known.
 *
 * If you need the filter name in input() get it in start() or
 * construct().  The filters name will never change after it's loaded.
 *
 * \return a string that you should only read.
 */
extern
const char* qsGetFilterName(void);


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


/** Remove the default command-line options for the filter.
 *
 * This function called in the filter module construct() function
 * allows the filter to opt out of having the default filter command
 * line options.
 *
 * \todo document qsRemoveDefaultFilterOptions()
 *
 * The default options are: --maxWrite, --maxRead, --config FILE, --name,
 * and ...
 *
 * qsRemoveDefaultFilterOptions() must be called in the filter's
 * construct() function.
 *
 * The option --name is not removed.
 */
extern
void qsRemoveDefaultFilterOptions(void);



//////////////////////////// Control/Parameter Stuff /////////////////////


// Parameters are owned by the filter that creates them.
//
// The value pointer is owned by the filter's thread.  If the filter is
// multi-threaded the filter insure that the value pointer stays valid.
//
//
//
// Parameter values are must be copied in the callbacks.

/** \defgroup parameters Filter Module Parameters
 *
 * quickstream manages two kinds of data:
 *
 *   1. stream data that is quickly flowing between filters in the stream,
 *      and the rate at which data flows is relatively large compared to
 *   2. control parameter data that is changing relatively slowly.  The
 *      size of the data in the parameter must be small, so that it may be
 *      copied to be passed to other threads to be used.
 *
 *  We generally avoid directly copying stream data, and we usually copy
 *  parameters.  If a parameter changes at the rate of stream data than it
 *  should not be a parameter, and it should be stream data, and the
 *  filter flow graph should be re-engineered.
 *
 *  The parameter data is shared between filters and external entities
 *  called controllers.  An obvious example control parameter would be a
 *  sound volume level on a sound channel.  The parameter could just be a
 *  single number factor that multiples an input stream channel to
 *  give an output stream channel.  For such a volume filter, using a
 *  "pass-through" buffer would be best and so no copying of the stream
 *  data would be required, it would just multiple it as it was passed
 *  through from the input port to the output port using the same ring
 *  buffer for input and output.
 *
 *  Of course the use of the quickstream control parameter thingy is not
 *  required, and a user can make their own "control parameter" thingy.
 *  But the benefit of using it is that you take advantage of controller
 *  modules that can extend parameters to addition interfaces like for
 *  example making the parameters readable and writable from a web app
 *  page, without the filter writer needing to write any browser client or
 *  server code.  So if a filter has implemented a quickstream parameter,
 *  that parameter can then be controlled from all the quickstream
 *  controllers that are available without knowing about said controllers.
 *
 *  The cost is this butt ugly parameter interface, these 5 functions, and
 *  the benefit is seamlessly extending the controlling of parameters to a
 *  library of widgets.  Such a paradigm is helpful in constructing the
 *  Internet of Things (IoT).
 *
 *  The parameters are small and copied between threads that are running
 *  different codes, so copying them in callback functions is straight
 *  forward and acceptable way to pass they between these independent
 *  computations.
 *
 *  Past parameter values are not queued, or stored by quickstream.  They
 *  are not time stamped.  They do not have a sample rate.  It's up to the
 *  filter or controller module write to add that kind of thing, if it is
 *  needed.
 *
 *  Parameters cannot be added or removed while the stream is flowing.
 *
 *  \todo examples linked here.
 *
 * @{
 */

/** parameter type
 *
 */
enum QsParameterType {

    None = 0,
    QsDouble = 1
};

/** qsParameterCreate() is called in a filter module construct() to create
 * a parameter
 *
 * \param pName is the name of the parameter.  This name only needs to be
 * unique to the filter module.
 *
 * \param setCallback is a function that is called when qsParameterSet()
 * is called in by another entity possibly in a different thread than one
 * used to call the filter input().  setCallback() will most likely be
 * called in a different thread than the filter's input() function thread.
 * So steps must be taken by the user to keep data consistent between
 * these different threads.  In the simplest case just atomically copying
 * the data to another variable in the setCallback.
 *
 * Calling setCallback() should not block.  If there is a blocking call
 * required it should queue up the request, and act later.  If
 * setCallback() does block quickstream will not brake, but it may become
 * slowstream.
 *
 * \param type is the parameter type.
 *
 * The \p value pointer will point to memory that is not owned by the
 * filter that is calling this function.  The memory should likely be
 * copied.
 *
 * The stream and filter name are not needed as parameters because the
 * filter module knows it's stream and filter name.
 *
 * \return 0 on success and 1 if the parameter already exists, and -1 if
 * there is an error like running out of memory and crap like that.
 */
extern
int qsParameterCreate(const char *pName, enum QsParameterType type,
        int (*setCallback)(void *value));



/** Destroy a filter module parameter
 *
 * \param pName is the name of the parameter.  This name only needs to be
 * unique to the filter module.
 *
 * \todo do we need this.
 */
extern
int qsParameterDestroy(const char *pName);


struct QsStream;

/** Register a callback to get a parameter value from outside the filter
 * module
 *
 * This function sets up a callback to be called every time the value of
 * the parameter changes as the filter that owns the parameter defines it.
 * The user must know what \p value is and how to use it.  The memory
 * pointed to by value should be copied if it is to be used after this
 * getCallback() returns.  The user must assume that getCallback will be
 * called in another thread, the owning filter module thread, so
 * getCallback() must be thread-safe.
 *
 * \param stream is the stream that the filter is in.
 *
 * \param filterName is the unique name that the filter has.  The filter
 * name is assigned to the filter when the filter module is loaded.  If
 * filterName is 0 than all filters will be considered.
 *
 * \param pName is the name of the parameter in that filter if pName is 0
 * all parameters will be considered.
 *
 * \param type is the type of parameter.  The \p getCallback() needs to
 * know what to do with the \p value.
 *
 * \param getCallback is the callback function.  This callback function
 * must understand the nature and size of what the parameter value is.  \p
 * getCallback() is called any time the parameter changes, which is when
 * qsParameterSet() is called.  getCallback() maybe called in a thread
 * that may be different than the thread being used by the caller.
 *
 * Calling getCallback() should not block.
 *
 * If the getCallback returns non-zero the callback will be removed.
 *
 * \return 0 on success and non-zero otherwise.
 */
extern
int qsParameterGet(const struct QsStream *stream, const char *filterName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            void *value, const struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type));


/** Set a parameter by calling the filter's callback, called from outside
 * the owning filter module
 *
 * This is called by an entity that is not the filter module that created
 * the parameter.  The filter can act however it chooses to, and may even
 * ignore the request.  In order for the user of this function to know the
 * real state of the parameter they must call qsParameterGet().
 * qsParameterSet() is just a request and may not really set the
 * parameter, it's up to the filter modules qsParameterCreate() callback
 * what really happens.
 *
 * The alternative of having a cross-thread queue may have had a simpler
 * owner filter interface, but would have added complexity and decreased
 * performance due to thread synchronization.  This code is much simpler.
 *
 * \param stream that owns the filter.
 *
 * \param filterName is the filter name that the filter was given at load
 * time.
 *
 * \param pName is the parameter name.
 *
 * \param value is a value from the calling entity.
 *
 * \return 0 on success and non-zero otherwise.
 */
extern
int qsParameterSet(const struct QsStream *stream,
        const char *filterName, const char *pName,
        enum QsParameterType type, void *value);


/** Push the value to the qsParameterGet() callbacks
 *
 * qsParameterPush() is used when the filter modules that owns the
 * parameter has set the parameter without a request from an external
 * qsParameterSet() call.
 *
 * \param pName is the parameter name.
 *
 * \param value is the value to set.
 *
 * \return 0 on success and non-zero otherwise.
 */
int qsParameterPush(const char *pName, void *value);


/** @}
 */


#ifdef __cplusplus
}
#endif

#endif // #ifndef __qsfilter_h__
