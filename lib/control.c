#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../include/quickstream/filter.h"
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "filterAPI.h" // struct QsJob *GetJob(void){}


// We make a weird ass Dictionary tree data structure that has a hierarchy
// structure.  The top level is the stream pointer.  The direct children
// of the stream pointer are keyed with the className and a value is a
// pointer to the filter or controller.  And so on like below:
//
// Dictionary node sub-tree hierarchy structure levels:
//
// 0. stream#:        value = stream pointer
// 1. ClassName       value = filter pointer or controller pointer
// 2. :get: or :set:  value = "there may be a use someday"
// 3. parameterName   value = current set or get value
// 4. :cbs:           value = ((void *) 1 or 2) if there is a subtree
// 5. callbacks keyed "0", "1", "2", ...
//
//
//  For filter Class = stdin-2:
//
//      s2: stdin-2 :get: freq1 :cbs: 0
//      s2: stdin-2 :set: freq1 :cbs:
//
//      s2: stdin-2 :get: freq2
//      s2: stdin-2 :set: freq2
//
//
//  For non-filter Controller Class = wombat:
//
//      s2: wombat :get: freq1
//      s2: wombat :set: freq1
//      s2: wombat :get: freq2
//      s2: wombat :set: freq2
//
//
int qsParameterCreate(struct QsStream *s, const char * Class,
        const char *name, void *value, bool exclusive) {

    struct QsDictionary *d;

    if(s == 0) {
        DASSERT(Class == 0);
        // This is a request from a filter input() function.
        struct QsFilter *f = GetFilter();
        ASSERT(f);
        s = f->stream;
        d = GetStreamDictionary(s); // now at level 0.
        int ret = qsDictionaryInsert(d,
                qsGetFilterName()/*key*/, f/*value*/, &d);
        // now at level 1.
        // It may be created, 0, or it may exist already, 1.
        ASSERT(ret == 0 || ret == 1);
    } else {
        DASSERT(GetFilter() == 0,
                "only filter input() call should "
                "be able to do this");
        ASSERT(Class);
        d = GetStreamDictionary(s); // now at level 0.
        // TODO: get controller pointer from controller module.
        int ret = qsDictionaryInsert(d, Class, "controllerPointer", &d);
        // It may be created, 0, or it may exist already, 1.
        // now at level 1.
        ASSERT(ret == 0 || ret == 1);
    }

    DASSERT(d);
    struct QsDictionary *dd;

    int ret = qsDictionaryInsert(d, ":get:", ":get:", &dd);
    // dd is now at level 2
    //
    // It may be created, 0, or it may exist already, 1.
    ASSERT(ret == 0 || ret == 1);

    ret = qsDictionaryInsert(dd, name, value, 0);

    if(ret == 1 && exclusive) {
        ERROR("control: s%" PRIu32 ":get:%s exists", s->id, name);
        // This may not be a big deal, depending on the use case.
        return -1; // error
    } else
        // dd is now at level 3
        //
        // It may be created, 0, or it may exist already, 1.
        ASSERT(ret == 0 || ret == 1);


    ret = qsDictionaryInsert(d, ":set:", ":set:", &dd);
    // dd is now at level 2
    //
    // It may be created, 0, or it may exist already, 1.
    ASSERT(ret == 0 || ret == 1);

    ret = qsDictionaryInsert(dd, name, value, 0);

    if(ret == 1 && exclusive) {
        ERROR("control: s%" PRIu32 ":set:%s exists", s->id, name);
        // This may not be a big deal, depending on the use case.
        return -1; // error
    } else
        // dd is now at level 3
        //
        // It may be created, 0, or it may exist already, 1.
        ASSERT(ret == 0 || ret == 1);


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
