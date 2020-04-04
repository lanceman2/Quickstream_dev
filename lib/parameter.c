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

    void *userData;

    int (*setCallback)(void *value, void *userData);
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
        int (*setCallback)(void *value, void *userData),
        void *userData) {

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
    ASSERT(ret == 0 || ret == 1);

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
    qsDictionarySetFreeValueOnDestroy(d);
    p->type = type;
    p->userData = userData;
    p->setCallback = setCallback;

    return 0;
}


struct GetCallback {

    int (*getCallback)(
            const void *value, struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData);
    void *userData;
};



int qsParameterGet(struct QsStream *s, const char *filterName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            const void *value, struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData) {


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

    struct GetCallback *gc = malloc(sizeof(*gc));
    ASSERT(gc, "malloc(%zu) failed", sizeof(*gc));
    gc->getCallback = getCallback;
    gc->userData = userData;

    ret = qsDictionaryInsert(d, key, gc, &d);
    ASSERT(ret == 0,
            "qsDictionaryInsert(,key=\"%s\",getCallback,) failed", key);
    DASSERT(d);
    qsDictionarySetFreeValueOnDestroy(d);


    return 0;
}


struct CallbackArgs {

    const void *value;
    struct QsStream *stream;
    const char *filterName;
    const char *pName;
    enum QsParameterType type;
};




static
int GetCallbackWapper(const char *key, struct GetCallback *gc,
        struct CallbackArgs *a) {

    if(key[0] == '\a')
        // Skip this first one.
        return 0;

    int (*getCallback)(
            const void *value, struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData) = gc->getCallback;

    return getCallback(a->value, a->stream, a->filterName,
            a->pName, a->type, gc->userData);
}



// Call all the controller Get() callbacks
static inline
void ParameterPushGets(struct QsStream *s,
        const char *filterName,
        const char *pName, void *value,
        struct QsParameter *p, struct QsDictionary *d) {

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


static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t parameterKey;

static void MakeKey(void) {
    CHECK(pthread_key_create(&parameterKey, 0));
}


int qsParameterSet(struct QsStream *s,
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
    struct QsFilter *f = 0;
    d = qsDictionaryFindDict(d, GetFilterLeafName(filterName, leaf), 0);
    if(!d) {
        ERROR("Filter \"%s\" not found in parameter tree", filterName);
        return -1; // error
    }
    f = qsDictionaryGetValue(d);
    DASSERT(f);

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

    // We need thread specific data to tell what filter this is when
    // setCallback() is called below.  
    CHECK(pthread_once(&keyOnce, MakeKey));

    // This needs to be re-entrant code.
    void *oldFilter = pthread_getspecific(parameterKey);
    CHECK(pthread_setspecific(parameterKey, f));

    // This may call qsParameterPush() or it may not, or qsParameterPush()
    // may be called later, after this call.  It's up to the filter module
    // when and if to call qsParameterPush().  It may not call it if the
    // parameter does not change due to this call.
    p->setCallback(value, p->userData);

    CHECK(pthread_setspecific(parameterKey, oldFilter));

    // Now we wait for this to have an effect.  The effect does not have
    // to be soon.

    return 0;
}



// This is call inside the filter setCallback()
//
// This is the effect we where waiting for from the filter module
// the owns the parameter.
//
int qsParameterPush(const char *pName, void *value) {

    // First get the filter pointer some how.

    // This function must be called from the filter module somewhere:
    //
    // So it may be called from the filter setCallback() from a
    // qsParameterSet() and if so this will get the filter object:
    
    CHECK(pthread_once(&keyOnce, MakeKey));
    
    struct QsFilter *f = pthread_getspecific(parameterKey);

    if(!f)
        // Or it may be called after the filter setCallback() in one of
        // the filter module functions start() stop() or input().
        // GetFilter() is a small wrapper that uses a different pthread
        // specific variable.
        f = GetFilter();

    // We must have the filter pointer now.
    DASSERT(f);
    DASSERT(f->stream);
    DASSERT(f->name);

    // Get stream dict:
    struct QsDictionary *d = GetStreamDictionary(f->stream);
    DASSERT(d);
    DASSERT(qsDictionaryGetValue(d) == f->stream);

    // Get the filter dict:
    char leafName[LEAFNAMELEN];
    d = qsDictionaryFindDict(d,
            GetFilterLeafName(f->name, leafName), 0);
    ASSERT(d);

    // Get parameter dict:
    struct QsParameter *p;
    d = qsDictionaryFindDict(d, pName, (void **) &p);
    ASSERT(d);
    DASSERT(p);

    if((d = qsDictionaryFindDict(d, "\a", 0)))
        ParameterPushGets(f->stream, f->name, pName, value, p, d);

    return 0;
}


static size_t
ForEachStream(struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            const void *value, struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData) {

    ASSERT(0, "Write this code!!!!!");

    return 0;
}


size_t qsParameterForEach(struct QsApp *app, struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            const void *value, struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData) {

    size_t ret = 0;


    if(!s) {
        ASSERT(app, "app must be set to call qsParameterForEach() "
                "if stream is not set");
        for(s = app->streams; s; s = s->next)
            ret += ForEachStream(s, filterName, pName, type,
                    callback, userData);
        return ret;
    }
    return ForEachStream(s, filterName, pName, type, callback, userData);
}
