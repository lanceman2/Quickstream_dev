#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The public installed user interfaces:
#include "../include/qsapp.h"
#include "../include/qsfilter.h"

// Private interfaces.
#include "./qs.h"
#include "./debug.h"


#if 0
static int Run(struct QsStream *s, struct QsFilter *f) {

    return 0;

}
#endif

// Run when user set 0 threads and 0 processes
//
// In this case each source should be calling something like a blocking
// read, otherwise if it is not a blocking read than this will use a lot
// of CPU resource like a spinning loop.
//
int stream_run_0p_0t(struct QsStream *s) {

    DASSERT(s->numSources, "");
    DASSERT(s->sources, "");
    DASSERT(s->sources[0], "");

    while(true) {
        for(uint32_t i=0; i<s->numSources; ++i) {
            DASSERT(s->sources[i],"");
            struct QsFilter *filter = s->sources[i];
            DASSERT(filter,"");
            DASSERT(filter->input, "");

            _qsInputFilter = filter;
            // Feed the source filter 0 bytes.
            enum QsFilterInputReturn ret;
            ret = filter->input(0, 0, 0);
            _qsInputFilter = 0;

            if(ret) {
                WARN("filter \"%s\" input() returned %d",
                        s->sources[i]->name, ret);
                // TODO: ?
                break;
            }
            switch(ret) {
                case QsFContinue:

                    break;
                case QsFFinished:

                    break;
                default:
                    WARN("filter \"%s\" input() returned "
                            "unknown enum QsFilterInputReturn %d",
                            s->sources[i]->name, ret);
            }
        }
    }

    return 0;
}
