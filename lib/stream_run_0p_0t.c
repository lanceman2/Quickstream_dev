#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The public installed user interfaces:
#include "../include/qsapp.h"

// Private interfaces.
#include "./qsapp.h"
#include "./debug.h"



// Run when user set no threads and no processes
//
// In this case each source should be calling something like a blocking
// read, otherwise if it is not a blocking read than this will use a lot
// of CPU resource like a spinning loop.
//
int stream_run_0p_0t(struct QsStream *s) {

#if 0
    bool flowing = true;
    do {
        for(uint32_t i=0; i<s->numSources; ++i) {
            DASSERT(s->sources[i],"");
            DASSERT(s->sources[i]->input,"");

            _qsCurrentFilter = s->sources[i];
            int ret = s->sources[i]->input(0, 0, 0);

            if(ret) {
                WARN("filter \"%s\" input() returned %d",
                        s->sources[i]->name, ret);
                // TODO: ?
                flowing = false;
            }
        }
    } while(flowing);
#endif
    return 0;
}