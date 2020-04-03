#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "../include/quickstream/filter.h"
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "filterAPI.h" // struct QsJob *GetJob(void){}



// Set and Get scenarios
//
//
// 1. A single controlling entity is a) receiving command requests,
//    and b) reporting state.
//
//    a) Many other entities submit Set requests with values.
//       - The actual parameter state value can only be gotten
//         with a Get request.
//
//    b) Many other entities submit Get requests to get values.
//
//
// 2. A single controlling entity just is reporting state.
//
//    a) The single controlling entity is the only entity that
//       may set the state values.  All other Set request must be
//       ignored.
//
//    b) Many other entities submit Get requests to get values.
//
//
//
// Entities are filters or controllers.
//
// Parameters cannot be added after the stream is flowing/running
// and so the stream's Dictionary can't change while the stream is
// flowing.
//


// We make a weird ass Dictionary tree data structure that has a
// hierarchical structure.  The top level is the stream pointer.  The
// direct children of the stream pointer are keyed with the className and
// a value is a pointer to the filter or controller.  And so on like
// below:
//
// Dictionary node sub-tree hierarchy structure levels:
//
// 0. stream#:        value = stream pointer
// 1. filterName      value = filter pointer or controller pointer
// 3. parameterName   value = struct Parameter *
// 4. getCallbacks keyed "0", "1", "2", ...
//
//
//  Example keys sequences for filterName = tx:
//
//  This is unfortunate, that we must append to the strings a terminator
//  like thing, otherwise names would be restricted in a not so nice way.
//
//  We use '\a' (bell) as a separator.  This assumes that users do not
//  use '\a' in the filter names.
//
//
//  Example keys sequences for filterName = tx:
//
//      s2\a tx\a freq1 \a 0  value = getCallback
//      s2\a tx\a freq1 \a 1  value = getCallback
//      s2\a tx\a freq1 \a 2  value = getCallback
//
//
//   freq1 value -> struct QsParameter 


struct QsParameter {

    enum QsParameterType type;

    int (*setCallback)(void *value);
};



static inline
const char *GetTypeString(enum QsParameterType type) {
    
    switch (type) {

        case QsDouble:
            return "double";
        default:
            ASSERT(0, "Unknown enum QsParameterType");
            return 0;
    }
}

#define LEAFNAMELEN  (_QS_FILTER_MAXNAMELEN + 2)


static inline
char *GetFilterLeafName(const char *filterName,
        char leafName[LEAFNAMELEN]) {
    snprintf(leafName, LEAFNAMELEN, "%s\a", filterName);
    return leafName;
}


int qsParameterCreate(const char *pName, enum QsParameterType type,
        int (*setCallback)(void *value)) {

    
    struct QsFilter *f = GetFilter();
    DASSERT(f);
    DASSERT(f->stream);
    DASSERT(f->name);

    // Get stream dict:
    struct QsDictionary *d = GetStreamDictionary(f->stream);
    DASSERT(qsDictionaryGetValue(d) == f->stream);

    // Create of get filter dict:
    char leafName[LEAFNAMELEN];
    int ret = qsDictionaryInsert(d,
            GetFilterLeafName(f->name, leafName), f, &d);
    ASSERT(ret != 0 || ret != 1);

    // Create parameter dict:
    ret = qsDictionaryInsert(d, pName, "p", &d);
    ASSERT(ret >= 0);
    if(ret) {
        ERROR("Parameter %s:%s already exists", f->name, pName);
        return 1;
    }
    struct QsParameter *p = malloc(sizeof(*p));
    ASSERT(p, "malloc(%zu) failed", sizeof(*p));
    qsDictionarySetValue(d, p);
    p->type = type;
    p->setCallback = setCallback;

    return 0;
}



int qsParameterGet(const struct QsStream *s, const char *filterName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            void *value, const struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type)) {


    DASSERT(s);
    DASSERT(filterName);
    DASSERT(filterName[0]);
    DASSERT(pName);
    DASSERT(pName[0]);

    // Get stream dict:
    struct QsDictionary *d = GetStreamDictionary(s);
    DASSERT(qsDictionaryGetValue(d) == s);

    // Get filter dict:
    char leaf[LEAFNAMELEN];
    d = qsDictionaryFindDict(d, GetFilterLeafName(filterName, leaf), 0);
    if(!d) {
        ERROR("Filter \"%s\" not found in parameter tree", filterName);
        return -1; // error
    }

    // Get parameter dict:
    struct QsParameter *p = 0;
    d = qsDictionaryFindDict(d, pName, (void*) &p);
    ASSERT(d);
    DASSERT(p);
    DASSERT(p->setCallback);

    if(p->type != type) {
        ERROR("Filter parameter \"%s:%s\" type \"%s\" "
                "is not requested type \"%s\"",
                filterName, pName, GetTypeString(p->type),
                GetTypeString(type));
        return -1; // error
    }

    int ret = qsDictionaryInsert(d, "\a", (void *) 1, &d);
    ASSERT(ret == 0 || ret == 1,
            "qsDictionaryInsert(,,\"getCallbacks\",) failed");

    uintptr_t index = (uintptr_t) qsDictionaryGetValue(d);

    char key[16];
    snprintf(key, 16, "%zu", index-1);
 
    qsDictionarySetValue(d, (void *) (++index));

    ret = qsDictionaryInsert(d, key, getCallback, 0);
    ASSERT(ret == 0,
            "qsDictionaryInsert(,key=\"%s\",getCallback,) failed", key);

    return 0;
}


struct CallbackArgs {

    const void *value;
    const struct QsStream *stream;
    const char *filterName;
    const char *pName;
    enum QsParameterType type;
};



static
int GetCallbackWapper(const char *key, const void *value,
        struct CallbackArgs *a) {

    if(key[0] == '\a')
        // Skip this first one.
        return 0;

    int (*getCallback)(
            const void *value, const struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type) = value;

    return getCallback(a->value, a->stream, a->filterName,
            a->pName, a->type);
}



// Call all the controller Get() callbacks
static inline
void ParameterPushGets(const struct QsStream *s,
        const char *filterName,
        const char *pName, void *value,
        struct QsParameter *p, struct QsDictionary *d) {

    DSPEW();

    struct CallbackArgs args = {
        value,
        s,
        filterName,
        pName,
        p->type
    };

    qsDictionaryForEach(d,
            (int (*) (const char *, const void *, void *))
            GetCallbackWapper, &args);
}


int qsParameterSet(const struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type, void *value) {

    DASSERT(s);
    DASSERT(filterName);
    DASSERT(filterName[0]);
    DASSERT(pName);
    DASSERT(pName[0]);
    DASSERT(value);


    // Get stream dict:
    struct QsDictionary *d = GetStreamDictionary(s);
    DASSERT(qsDictionaryGetValue(d) == s);

    // Get filter dict:
    char leaf[LEAFNAMELEN];
    d = qsDictionaryFindDict(d, GetFilterLeafName(filterName, leaf), 0);
    if(!d) {
        ERROR("Filter \"%s\" not found in parameter tree", filterName);
        return -1; // error
    }

    // Get parameter dict:
    struct QsParameter *p = 0;
    d = qsDictionaryFindDict(d, pName, (void*) &p);
    ASSERT(d);
    DASSERT(p);
    DASSERT(p->setCallback);

    if(p->type != type) {
        ERROR("Filter parameter \"%s:%s\" type \"%s\" "
                "is not requested type \"%s\"",
                filterName, pName, GetTypeString(p->type),
                GetTypeString(type));
        return -1; // error
    }

    p->setCallback(value);

    d = qsDictionaryFindDict(d, "\a", 0);

    if(d)
        ParameterPushGets(s, filterName, pName, value, p, d);

    return 0;
}




int qsParameterPush(const char *pName, void *value) {

    return 0;
}


int qsParameterDestroy(const char *pName) {

    return 0;
}
