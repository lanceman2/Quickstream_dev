#ifndef __qsfilter_h__
#define __qsfilter_h__

//
// quickstream filter interfaces that the user may provide in their
// filter module.
//


int constructor(void);

int destructor(void);

int start(int numInChannels, int numOutChannels);

int stop(int numInChannels, int numOutChannels);

// This function is required for all quickstream filter modules.
int input(void *buffer, size_t len, int inputChannelNum);


extern int qsGetFilterID(void);


#endif // #ifndef __qsfilter_h__

