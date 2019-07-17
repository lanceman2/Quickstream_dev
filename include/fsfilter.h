#ifndef __fsfilter_h__
#define __fsfilter_h__

//
// FSF fast stream filter interfaces that the user may provide in their
// filter module.
//


int constructor(void);

int destructor(void);

int start(int numInChannels, int numOutChannels);

int stop(int numInChannels, int numOutChannels);

// This function is required for all faststream filter modules.
int input(void *buffer, size_t len, int inputChannelNum);


extern int fsGetFilterID(void);


#endif // #ifndef __fsfilter_h__

