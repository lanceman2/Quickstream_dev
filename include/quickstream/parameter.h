#ifndef __qsparameter_h__
#define __qsparameter_h__

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
 * We generally avoid directly copying stream data, and we usually copy
 * parameters.  If a parameter changes at the rate of stream data than it
 * should not be a parameter, and it should be stream data, and the
 * filter flow graph should be re-engineered.
 *
 * The parameter data is shared between filters and external entities
 * called controllers.  An obvious example control parameter would be a
 * sound volume level on a sound channel.  The parameter could just be a
 * single number factor that multiples an input stream channel to give an
 * output stream channel.  For such a volume filter, using a
 * "pass-through" buffer would be best and so no copying of the stream
 * data would be required, it would just multiple it as it was passed
 * through from the input port to the output port using the same ring
 * buffer for input and output.
 *
 * Any parameter is owned by either a filter or a controller.  When the
 * owning filter or controller are destroyed, so is the parameter.  That's
 * just how we define owner of a parameter.
 *
 * Some parameters may not be requested to be set by code that is not the
 * owner.  This kind of parameter just ignores set requests bye not having
 * a setCallback, or having a setCallback that does not use the value
 * passed in to set the value.  A good example of such a parameter is
 * filter through-put, which you would want to be set by something that
 * is not measuring the through-put.
 *
 * This parameter API is an synchronous API that enables non-blocking
 * inter-filter and controller-filter communication, so long as the user
 * does not use callback functions that block.  A controller is a code
 * that can access this API and is not a filter module.
 *
 * Of course the use of the quickstream control parameter thingy is not
 * required, and a user can make their own "control parameter" thingy.
 * But the benefit of using it is that you take advantage of controller
 * modules that can extend parameters to have additional interfaces like
 * for example making the parameters readable and writable from a web app
 * page, without the filter writer needing to write any browser client or
 * server code.  So if a filter has implemented a quickstream parameter,
 * that parameter can then be controlled from all the quickstream
 * controllers that are available without knowing about said
 * controllers.
 *
 * Parameter passed between entities with the callback in this API are
 * intended to be copied in the callbacks.  So ya, the parameters must be
 * small, like two floats; hence stack memory can be used for passing the
 * value, and it can be copied to local object state from there without
 * mutexes-n-shit.  You may need mutexes for your local object state, but
 * not for the passed parameter values.
 *
 * The cost is this butt ugly parameter interface, these 6 functions; and
 * the benefit is seamlessly extending the controlling of parameters to a
 * library of controllers.  Such a paradigm is helpful in constructing the
 * Internet of Things (IoT).
 *
 * The parameters are small and copied between threads that are running
 * different codes, so copying them in callback functions is straight
 * forward and acceptable way to pass they between these independent
 * computations.
 *
 * Past parameter values are not queued, or stored by quickstream.  They
 * are not time stamped.  They do not have a sample rate.  It's up to the
 * filter or controller module writer to add that kind of thing, if it is
 * needed.
 *
 * Parameters cannot be added or removed while the stream is flowing.
 *
 *  \todo examples linked here.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


/** parameter type
 *
 */
enum QsParameterType {

    None = 0,
    QsDouble = 1,
    QsUint64 = 2,
    QsNew = 3
};


/** qsParameterCreate() is called in a filter in construct() or in
 * a controller module to create a parameter
 *
 * In general the filter or controller must exist for the parameter to
 * exist, because the state of the parameter is not necessarily just the
 * value of a variable, but can be defined by a complex algorithm.
 *
 * \param pName is the name of the parameter.  This name only needs to be
 * unique to the filter module.
 *
 * \param type is the parameter type.  type tells you how to deal with the
 * \p value.
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
 * required it should queue up the request, and act later. This
 * qsParameter class does not provide queuing of set requests, all the
 * functions just call the callbacks while keeping it ordered and
 * synchronous, and if the user needs queuing the user can add it on top
 * of this interface without much work.   If setCallback() does block
 * quickstream will not brake, but it may become slowstream.
 *
 * \param cleanup is a clean-up function that is called to do things like
 * free memory associated with the parameter after the parameter is no
 * longer needed.  \p cleanup may be 0 if no clean-up is needed.
 *
 * Parameters are destroyed when the owning filter or controller is
 * unloaded, and \p cleanup() is called.
 *
 * \param userData is passed to the setCallback() function every time it
 * is called.
 *
 * The \p value pointer will point to memory that is not owned by the
 * filter that is calling this function.  The memory should likely be
 * copied.
 *
 * The stream and filter name are not needed as parameters because the
 * filter module knows it's stream and filter name.
 *
 * \return 0 on success and 1 if the parameter already exists.
 * returns -1 if the filter or controller module is not found.
 */
extern
int qsParameterCreate(const char *pName, enum QsParameterType type,
        int (*setCallback)(void *value, const char *pName,
            void *userData),
        void (*cleanup)(const char *pName, void *userData),
        void *userData);


struct QsFilter;
struct QsStream;
struct QsApp;


/** create a parameter that is associated with a particular filter
 *
 * This is the same as qsParameterCreate() except that this is not
 * required to be called from the filter construct() function.  This
 * should be called before the stream is flowing.
 *
 * This can be called from outside a filter module's code, like in a
 * controller module.
 *
 * \see qsParameterCreate()
 *
 * \param filter is the filter that will own the parameter.  The filter
 * may know nothing about the parameter, but when the filter is destroyed
 * the parameter is destroyed with it.
 *
 *\param pName is the name of the parameter.  This name only needs to be
 * unique to the filter module.
 *
 * \param type is the parameter type.  That tells you how to deal with
 * the \p value.  quickstream does not have a parameter typing system,
 * just this integer \p type number.
 *
 * \param setCallback is a function that is called when qsParameterSet()
 * is called in by another entity possibly in a different thread than one
 * used to call the filter input().
 *
 * \param cleanup is a clean-up function that is called to do things like
 * free memory associated with the parameter after the parameter is no
 * longer needed.  \p cleanup may be 0 if no clean-up is needed.
 *
 * \param userData is passed to the setCallback() function every time it
 * is called.
 *
 * The \p value pointer will point to memory that is not owned by the
 * filter that is calling this function.  The memory should likely be
 * copied.
 *
 * The stream and filter name are not needed as parameters because the
 * filter module knows it's stream and filter name.
 *
 * \return 0 on success and 1 if the parameter already exists.  If the
 * parameter already exists then nothing will change.
 */
extern
int qsParameterCreateForFilter(struct QsFilter *filter,
        const char *pName, enum QsParameterType type,
        int (*setCallback)(void *value, const char *pName,
            void *userData),
        void (*cleanup)(const char *pName, void *userData),
        void *userData);



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
 * \param streamOrApp is a pointer to the stream or app that the
 * corresponding filter or controller, respectively, belongs
 * to.
 *
 * \param ownerName is the unique name that the filter or controller that
 * owns this parameter.  The filter name is assigned to the filter when
 * the filter module is loaded.  The controller name is assigned to the
 * filter when the filter module is loaded.   If ownerName is 0 than all
 * filters will be considered.
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
 * \param userData is passed to the getCallback() every time it is called.
 *
 * Calling getCallback() should not block.
 *
 * If the getCallback returns non-zero the callback will be removed.
 *
 * \return 0 on success and non-zero otherwise.
 */
extern
int qsParameterGet(void *streamOrApp, const char *ownerName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            const void *value, struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData), void *userData);


/** Set a parameter by calling the filter's callback, called from outside
 * the owning filter or controller module
 *
 * This is called by an entity that is not the filter or controller module
 * that created the parameter.  The filter (or controller) can act however
 * it chooses to, and may even ignore the request.  In order for the user
 * of this function to know the real state of the parameter they must call
 * qsParameterGet().  qsParameterSet() is just a request and may not
 * really set the parameter, it's up to the filter (or controller) modules
 * qsParameterCreate() callback what really happens.
 *
 * The alternative of having a cross-thread queue may have had a simpler
 * owner filter interface, but would have added complexity and decreased
 * performance due to thread synchronization.  This code is much simpler.
 *
 * \param streamOrApp is the stream or app that owns the filter or
 * controller respectively.
 *
 * \param ownerName is the filter (or controller) name that the filter (or
 * controller) was given at load time.
 *
 * \param pName is the parameter name.
 *
 * \param type is the parameter type.  That tells you how to deal with
 * the \p value.  quickstream does not have a parameter typing system,
 * just this integer \p type number.
 *
 * \param value is a value from the calling entity.
 *
 * \return 0 on success and non-zero otherwise.
 */
extern
int qsParameterSet(void *streamOrApp, const char *ownerName,
        const char *pName,
        enum QsParameterType type, void *value);


/** Push the value to the qsParameterGet() callbacks in other modules
 *
 * qsParameterPush() is used when the filter modules that owns the
 * parameter has set the parameter to push the value via the registered
 * getCallbacks from qsParameterGet().
 *
 * By having the filter modules call this in where setCallback the values
 * can just point to variables on the stack, and this makes the
 * getCallbacks from qsParameterGet() thread safe.  The data passed as
 * arguments to getCallbacks needs to be copied in the getCallbacks in
 * over to be used in a later call in that module.
 *
 * \param pName is the parameter name.
 *
 * \param value is the value to set.  This value can be a pointer to a
 * stack allocated variable.  The getter callbacks are called in this
 * function.
 *
 * \return 0 on success and non-zero otherwise.
 */
int qsParameterPush(const char *pName, void *value);



/** Iterate through the parameters via a callback function
 *
 * This function has a butt load of argument parameters but lots of them
 * can be 0.  One of argument parameters \p app or \p stream must be
 * non-zero.
 *
 * \param app is ignored unless \p stream is 0.  If \p app is non-zero and
 * \p stream is zero iterated through all selected parameters in all
 * streams in  this \p app.
 *
 * \param stream if not 0, restrict the range of the parameters to iterate
 * through to just parameters in this stream.
 *
 * \param filterName if not 0, restrict the range of the parameters to
 * iterate through to just parameters in the filter with this name.
 *
 * \param pName if not 0, restrict the range of the parameters to iterate
 * through to just parameters with this name.
 *
 * \param type if not 0, restrict the range of the parameters to iterate
 * through to just parameters with this type.
 *
 * \param callback is the callback function that is called with each
 * parameter and with all of the arguments set.  If \p callback() returns
 * non-zero than the iteration will stop, and that will be the last time
 * \p callback() is called for this call to \p qsParameterForEach().
 *
 * \param userData is user data that is passed to the callback every
 * time
 * it is called.
 *
 * \return the number of parameters that have been iterated through; or
 * the same as, the number of times \p callback() is called.
 */
extern
size_t qsParameterForEach(struct QsApp *app, struct QsStream *stream,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            struct QsStream *stream,
            const char *filterName, const char *pName,
            enum QsParameterType type, void *userData),
        void *userData);

/** @}
 */

#ifdef __cplusplus
}
#endif


#endif //#ifndef __qsparameter_h__
