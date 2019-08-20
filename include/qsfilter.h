#ifndef __qsfilter_h__
#define __qsfilter_h__

#include <inttypes.h>
//
// quickstream filter interfaces that the user may provide in their
// filter module.
//


int constructor(void);

int destructor(void);

int start(uint32_t numInChannels, uint32_t numOutChannels);

int stop(uint32_t numInChannels, uint32_t numOutChannels);

// This function is required for all quickstream filter modules.
int input(void *buffer, size_t len, uint32_t inputChannelNum);


extern int qsGetFilterID(void);


#endif // #ifndef __qsfilter_h__

