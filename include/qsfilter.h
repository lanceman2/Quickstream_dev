#ifndef __qsfilter_h__
#define __qsfilter_h__

#include <inttypes.h>
//
// quickstream filter interfaces that the user may provide in their
// filter module.
//

/** \file
 *
 * These are the quickstream filter interfaces that the user may provide
 * in their filter module.
 */

/** Optional constructor function
 *
 * This function, if present, is called only once just after the filter
 * is loaded.
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int constructor(void);


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


/** Required filter input work function
 *
 * \return 0 on success
 *
 * \todo figure out more return codes and what they mean
 */
int input(void *buffer, size_t len, uint32_t inputChannelNum);



extern int qsGetFilterID(void);


#endif // #ifndef __qsfilter_h__

