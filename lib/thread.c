#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "./qs.h"
#include "./debug.h"



struct QsThread *qsStreamThreadCreate(struct QsStream *s) {

    DASSERT(s, "");

    struct QsThread *t = calloc(1, sizeof(*t));
    ASSERT(t, "calloc(1,%zu) failed", sizeof(*t));

    t->stream = s;

    return t; // success
}


int qsThreadDestroy(struct QsThread *t) {

    DASSERT(t, "");
    DASSERT(t->stream, "");
    DASSERT(t->stream->app, "");

    // Go through all filters and remove this thread.
    // We could have gone through just the filters in the
    // corresponding stream, but this adds extra error checking
    // and there no "from" and "to" filter list redundancy.
    for(struct QsFilter *f = t->stream->app->filters; f; f=f->next)
        if(f->thread == t) {
            DASSERT(f->stream == t->stream, "");
            f->thread = 0;
        }

#ifdef DEBUG
    memset(t, 0, sizeof(*t));
#endif

    free(t);
    return 0; // success
}


int qsThreadAddFilter(struct QsThread *t, struct QsFilter *f) {

    DASSERT(t, "");
    DASSERT(t->stream, "");
    DASSERT(f, "");
    DASSERT(f->stream, "");
    // this filter must be connected in this threads' stream
    DASSERT(f->stream == t->stream, "");

    // We set this filters thread to this one.
    // If it had one before now, we just changed it.
    f->thread = t;

    return 0; // success
}
