#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../include/quickstream/filter.h"
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "filterAPI.h" // struct QsJob *GetJob(void){}




int qsParameterCreate(struct QsStream *s, const char * Class,
        const char *name, void *value) {

    if(s == 0) {
        DASSERT(Class == 0);
        // This may be a request from a filter input() function.
        struct QsFilter *f = GetFilter();
        ASSERT(f);
        s = f->stream;
        Class = qsGetFilterName();
    }
#ifdef DEBUG
    else
        DASSERT(GetFilter() == 0,
                "only filter input() call should "
                "be able to do this");
#endif

    DASSERT(s);
    DASSERT(s->app);

    struct QsDictionary *d = GetStreamDictionary(s);
    DASSERT(d);

    int ret = qsDictionaryInsert(d, Class, "Class", 0);
    // It may be created, 0, or it may exist already, 1.
    ASSERT(ret == 0 || ret == 1);

    // Add a "get" and a "set" sub-tree for this stream
    // in the app dictionary.
    struct QsDictionary *get;
    ret = qsDictionaryInsert(d, "get:", "get:", &get);
    // It may be created, 0, or it may exist already, 1.
    ASSERT(ret == 0 || ret == 1);

    struct QsDictionary *set;
    ret = qsDictionaryInsert(d, "set:", "set:", &set);
    // It may be created, 0, or it may exist already, 1.
    ASSERT(ret == 0 || ret == 1);


    // MORE HEREEEEEEEEEEEEEEEEEEEE


    return 0; // success
}


void *qsParameterGet(struct QsStream *s, const char * Class,
        const char *name,
        int (*callback)(void *retValue)) {


    return 0;
}


int qsParameterSet(struct QsStream *s, const char * Class,
        const char *name, void *value,
        int (*callback)(void *retValue)) {


    return 0; // success
}
