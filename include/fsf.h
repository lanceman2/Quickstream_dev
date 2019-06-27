#ifndef __FSF_H__
#define __FSF_H__
//
// FSF fast stream filter interfaces that the user may provide in their
// filter module.
//


int constructor(void);

int destructor(void);

int start(int numInChannels, int numOutChannels);

int stop(int numInChannels, int numOutChannels);

// This function is required.
int input(void *buffer, size_t len, int inputChannelNum);


extern int fsGetFilterID(void);



#endif // #ifndef __FSF_H__
