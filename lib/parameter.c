#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
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
//      stream-0 tx freq
//
//   freq value -> struct Parameter 



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


struct GetCallback {

    int (*getCallback)(
            const void *value,
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData);
    void *userData;
};


struct Parameter {

    enum QsParameterType type;

    void *userData;

    int (*setCallback)(void *value, const char *pName, void *userData);

    // realloc()ed array.
    size_t numGetCallbacks;
    struct GetCallback *getCallbacks;
};


static void FreeQsParameter(struct Parameter *p) {

    //DSPEW("Freeing Parameter");
    DASSERT(p);
    if(p->getCallbacks) {
        DASSERT(p->numGetCallbacks);
#ifdef DEBUG
        memset(p->getCallbacks, 0, p->numGetCallbacks *
                sizeof(*p->getCallbacks));
#endif
        free(p->getCallbacks);
    }
#ifdef DEBUG
    memset(p, 0, sizeof(*p));
#endif
    free(p);
}



int qsParameterCreateForFilter(struct QsFilter *f,
        const char *pName, enum QsParameterType type,
        int (*setCallback)(void *value, const char *pName,
            void *userData),
        void *userData) {

    DASSERT(f);
    DASSERT(f->name);
    DASSERT(f->parameters);

    // Create parameter dictionary entry.
    struct QsDictionary *d;
    int ret = qsDictionaryInsert(f->parameters, pName, "p", &d);
    ASSERT(ret >= 0);
    if(ret) {
        ERROR("Parameter %s:%s already exists", f->name, pName);
        return 1;
    }
    DASSERT(d);

    // Create and add the parameter data to this parameter dict
    struct Parameter *p = malloc(sizeof(*p));
    ASSERT(p, "malloc(%zu) failed", sizeof(*p));
    memset(p, 0, sizeof(*p));
    qsDictionarySetValue(d, p);
    qsDictionarySetFreeValueOnDestroy(d, (void (*)(void *))FreeQsParameter);
    p->type = type;
    p->userData = userData;
    p->setCallback = setCallback;

    return 0;
}



int qsParameterCreate(const char *pName, enum QsParameterType type,
        int (*setCallback)(void *value, const char *pName,
            void *userData),
        void *userData) {

    struct QsFilter *f = GetFilter();
    DASSERT(f);
    ASSERT(f->mark == _QS_IN_CONSTRUCT, "qsParameterCreate() "
            "must be called in a filter module construct()");

    return qsParameterCreateForFilter(f, pName, type,
            setCallback, userData);
}



int qsParameterGet(struct QsStream *s, const char *filterName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            const void *value,
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData) {


    DASSERT(s);
    DASSERT(filterName);
    DASSERT(filterName[0]);
    DASSERT(pName);
    DASSERT(pName[0]);

    struct QsFilter *f = qsFilterFromName(s, filterName);
    if(!f) {
        WARN("Filter named \"%s\" not found", filterName);
        return 1; // error
    }
    DASSERT(f->parameters);

    // Get parameter dict:
    struct Parameter *p = 0;
    struct QsDictionary *d = qsDictionaryFindDict(f->parameters,
            pName, (void*) &p);

    if(!d) {
        WARN("Parameter \"%s:%s\" not found", filterName, pName);
        return 2; // error
    }

    DASSERT(p);
    DASSERT(p->setCallback);

    if(p->type != type) {
        ERROR("Parameter \"%s:%s\" type \"%s\" "
                "is not requested type \"%s\"",
                filterName, pName, GetTypeString(p->type),
                GetTypeString(type));
        return 3; // error
    }

    size_t i = p->numGetCallbacks;

    p->getCallbacks = realloc(p->getCallbacks,
            (i+1)*sizeof(*p->getCallbacks));
    ASSERT(p->getCallbacks, "realloc(,%zu) failed",
            (i+1)*sizeof(*p->getCallbacks));
    struct GetCallback *gc = p->getCallbacks + i;
    gc->getCallback = getCallback;
    gc->userData = userData;
    ++p->numGetCallbacks;

    return 0; // success
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

    struct QsFilter *f = qsFilterFromName(s, filterName);
    if(!f) {
        WARN("Filter named \"%s\" not found", filterName);
        return 1; // error
    }
    DASSERT(f->parameters);

    // Get parameter dict:
    struct Parameter *p = 0;
    struct QsDictionary *d = qsDictionaryFindDict(f->parameters,
            pName, (void*) &p);
    if(!d) {
        WARN("Parameter \"%s:%s\" not found", filterName, pName);
        return 2; // error
    }

    DASSERT(p);
    DASSERT(p->setCallback);

    if(p->type != type) {
        ERROR("Filter parameter \"%s:%s\" type \"%s\" "
                "is not requested type \"%s\"",
                filterName, pName, GetTypeString(p->type),
                GetTypeString(type));
        return 3; // error
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
    p->setCallback(value, pName, p->userData);

    CHECK(pthread_setspecific(parameterKey, oldFilter));

    // Now we wait for this to have an effect.  The effect does not have
    // to be soon.

    return 0; // success
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

    // Get parameter dict:
    struct Parameter *p = 0;
    struct QsDictionary *d = qsDictionaryFindDict(f->parameters,
            pName, (void*) &p);
    if(!d) {
        WARN("Parameter \"%s:%s\" not found", f->name, pName);
        return 2; // error
    }

    DASSERT(p);
    DASSERT(p->setCallback);

    struct GetCallback *gcs = p->getCallbacks;
    size_t num = p->numGetCallbacks;
    for(size_t i=0; i<num; ++i)
        gcs[i].getCallback(value, f->stream,
                (const char *) f->name,
                pName, p->type, gcs[i].userData);

    return 0;
}


struct ParameterCBArgs {

    struct QsFilter *filter;
    const char *filterName;
    enum QsParameterType type;
    void *userData;
    int (*callback)(
        struct QsStream *stream,
        const char *filterName, const char *pName, 
        enum QsParameterType type, void *userData);
    bool *done;
};


static
int ParameterForEach(const char *key, void *value,
        struct ParameterCBArgs *cbArgs) {

    struct QsFilter *f = cbArgs->filter;
    const struct Parameter *p = value;
    DASSERT(f);

    int ret = 0;

    if((void *) f != value &&
            (!cbArgs->type || cbArgs->type == p->type)) {
        ret = cbArgs->callback(f->stream, f->name, key,
                p->type, cbArgs->userData);
        if(ret) *cbArgs->done = true;
    }

    return ret;
}


static size_t
ForParameterDict(struct QsFilter *f, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData, bool *done) {

    DASSERT(f->parameters);

    if(pName) {
        struct Parameter *p = qsDictionaryFind(f->parameters, pName);
        if(!p) {
            WARN("Parameter \"%s:%s\" not found", f->name, pName);
            return 0;
        }
        if(!type || type == p->type) {
            if(callback(f->stream, f->name, pName, p->type, userData))
                *done = true;
            return 1;
        }
        return 0;
    }

    struct ParameterCBArgs args = {
        f,
        f->name,
        type,
        userData,
        callback,
        done
    };

    // Subtract 1 for the call with the filter.
    return qsDictionaryForEach(f->parameters, 
            (int (*)(const char *, void *, void *))
            ParameterForEach, &args) - 1;
}


static size_t
ForStreamParameters(struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData, bool *done) {

    DASSERT(s);

    if(!filterName) {
        // We go through all filters in this stream.
        size_t ret = 0;
        for(struct QsFilter *f = s->filters; f; f = f->next) {
            ret += ForParameterDict(f, pName, type,
                    callback, userData, done);
            if(*done) break;
        }
        return ret;
    }

    struct QsFilter *f = qsFilterFromName(s, filterName);

    // Just this one filter.
    return ForParameterDict(f, pName, type, callback, userData, done);
}


size_t qsParameterForEach(struct QsApp *app, struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData) {

    bool done = false;

    if(!s) {
        size_t ret = 0;
        ASSERT(app, "app must be set to call qsParameterForEach() "
                "if stream is not set");
        // We go through all streams in this app.
        for(s = app->streams; s; s = s->next) {
            ret += ForStreamParameters(s,
                    filterName, pName, type,
                    callback, userData, &done);
            if(done) break;
        }
        return ret;
    }
    // Just this one stream.
    return ForStreamParameters(s, filterName,
            pName, type, callback, userData, &done);
}
