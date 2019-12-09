#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

#include "./qs.h"
#include "../include/qsfilter.h"
#include "./debug.h"



uint32_t singleThreadFlow(struct QsStream *s) {


    for(uint32_t i=0; i<s->numSources; ++i) {
        DASSERT(s->sources[i],"");
        struct QsFilter *filter = s->sources[i];
        DASSERT(filter,"");
        DASSERT(filter->input, "");

        // TODO:  FLOW

        // Check stream

        DSPEW();

    }

    return 0; // success
}

