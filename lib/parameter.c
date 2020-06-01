#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <errno.h>
#include <regex.h>

#include "../include/quickstream/filter.h"
#include "../include/quickstream/parameter.h"
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


// Get callbacks are called when qsParameterPush() is called.
struct GetCallback {

    int (*getCallback)(
            const void *value,
            void *streamOrApp,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData);
    void *userData;
    uint32_t flags;
};


struct QsParameter {

    enum QsParameterType type;

    void *userData; // that was passed to qsParameterCreate*()

    int (*setCallback)(struct QsParameter *parameter,
            void *value, const char *pName, void *userData);

    void (*cleanup)(const char *pName, void *userData);

    // realloc()ed array.
    size_t numGetCallbacks;
    struct GetCallback *getCallbacks;

    // The root dictionary object that contains this parameter.
    // In either a filter or a controller.
    struct QsDictionary *dict;

    void *streamOrApp;

    const char *filterName; // or controller name.
    char *pName;
};


static void FreeParameter(struct QsParameter *p) {

    DASSERT(p);
    DASSERT(p->pName);
    DSPEW("Freeing Parameter \"%s\"", p->pName);

    if(p->cleanup)
        p->cleanup(p->pName, p->userData);

    free(p->pName);
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


int
qsParameterDestroy(struct QsParameter *p) {

    return qsDictionaryRemove(p->dict, p->pName);
}


struct RemovalMarker {

    struct QsParameter *stack;
    regex_t regex;
};


static int MarkForRemoval(const char *pName, struct QsParameter *p,
        struct RemovalMarker *rm) {

    if(regexec(&rm->regex, pName, 0, NULL, 0) == 0) {
        // This parameter matched.
        //
        // We borrow the setCallback pointer (since we don't need it any
        // more) to mark this parameter by having it point to the next
        // parameter in a stack list that we will destroy after we
        // iterate through the dictionary.
        if(rm->stack) {
            p->setCallback = (int (*)(struct QsParameter *,
                    void *, const char *, void *)) rm->stack;
            rm->stack = p;
        } else {
            rm->stack = p;
            p->setCallback = 0; // null terminate.
        }
    }
    return 0;
}


int
qsParameterDestroyForFilter(struct QsFilter *f, const char *pName,
        uint32_t flags) {

    DASSERT(f);
    DASSERT(f->parameters);
    DASSERT(pName);
    DASSERT(pName[0]);

    if(! (flags & QS_PNAME_REGEX))
        return qsDictionaryRemove(f->parameters, pName);

    // pName is a regex (regular expression string).

    struct RemovalMarker rm = { 0 };

    int ret = regcomp(&rm.regex, pName, REG_EXTENDED);
    if(ret == REG_ESPACE)
        ASSERT(0, "regcomp(,\"%s\",REG_EXTENDED) failed", pName);
    if(ret) {
        ERROR("Bad regular expression \"%s\"", pName);
        return -3;
    }

    qsDictionaryForEach(f->parameters,
                (int (*) (const char *key, void *value,
                        void *userData)) MarkForRemoval,
                &rm);

    int count = 0;
    // Now look for "marks", they are in a stack list to remove.
    struct QsParameter *p = rm.stack;
    while(p) {
        ++count;
        // We borrowed the setCallback pointer to make a stack, seeing as
        // we do not need it anymore, and we like small data structures.
        struct QsParameter *next = (struct QsParameter *) p->setCallback;
        qsDictionaryRemove(f->parameters, p->pName);
        // p is no longer points to valid memory, but next is valid.
        p = next;
    }

    return count;
}


static
struct QsDictionary *GetParameterDictionary(void *as, const char *name) {

    DASSERT(as);

    if(((struct QsStream *)as)->type == _QS_STREAM_TYPE) {

        struct QsFilter *f =
            qsFilterFromName((struct QsStream *)as, name);
        if(!f) {
            WARN("Filter named \"%s\" not found", name);
            return 0; // error
        }
        DASSERT(f->parameters);
        return f->parameters;
    } else {
        DASSERT(((struct QsApp *)as)->type == _QS_APP_TYPE);
        struct QsController *c =
            qsDictionaryFind(((struct QsApp *)as)->controllers, name);
        if(!c) {
            WARN("Controller named \"%s\" not found", name);
            return 0; // error
        }
        DASSERT(c->parameters);
        return c->parameters;
    }
}


int RemoveCallbacksForRestart(const char *key,
        struct QsParameter *p, void *userData) {

    if(p->numGetCallbacks == 0) return 0;

    DASSERT(p->getCallbacks);

    // Squeeze all the getCallbacks that we'll be keeping to a condensed
    // array.
    size_t newNum=0;
    for(size_t j=0; j<p->numGetCallbacks;) {
        // Keep going until we find one to keep:
        while(j<p->numGetCallbacks &&
                !(p->getCallbacks[j].flags & QS_KEEP_AT_RESTART)) ++j;
        if(j==p->numGetCallbacks) break;
        // We keep j-th one in the newNum position.
        if(newNum != j)
            memcpy(&(p->getCallbacks[newNum]), &(p->getCallbacks[j]),
                    sizeof(*p->getCallbacks));
        // else newNum == j so we do not need to copy it.

        ++newNum;
        ++j;
    }

    if(p->numGetCallbacks != newNum && newNum) {
        p->getCallbacks = realloc(p->getCallbacks, newNum*
                sizeof(*p->getCallbacks));
        ASSERT(p->getCallbacks, "realloc(%p,%zu) failed",
                p->getCallbacks, newNum*
                sizeof(*p->getCallbacks));
    } else if(newNum == 0) {
#ifdef DEBUG
        memset(p->getCallbacks, 0, p->numGetCallbacks*
                sizeof(*p->getCallbacks));
#endif
        free(p->getCallbacks);
    }
    p->numGetCallbacks = newNum;
    return 0;
}


void
_qsParameterRemoveCallbacksForRestart(struct QsFilter *f) {

    qsDictionaryForEach(f->parameters,
            (int (*) (const char *key, void *value,
                    void *userData)) RemoveCallbacksForRestart, 0);
}


static
struct QsParameter *
CreateParameter(struct QsDictionary *dict/*root of dictionary*/,
        void *streamOrApp,
        struct QsDictionary *entry, const char *fName,
        const char *pName,
        int (*setCallback)(struct QsParameter *parameter,
            void *value, const char *pName, void *userData),
        void (*cleanup)(const char *pName, void *userData),
        void *userData,
        enum QsParameterType type) {

    DASSERT(pName);
    DASSERT(pName[0]);

    struct QsParameter *p = malloc(sizeof(*p));
    ASSERT(p, "malloc(%zu) failed", sizeof(*p));
    memset(p, 0, sizeof(*p));
    qsDictionarySetValue(entry, p);
    qsDictionarySetFreeValueOnDestroy(entry, (void(*)(void *))FreeParameter);
    p->pName = strdup(pName);
    p->streamOrApp = streamOrApp;
    // This fName string should exist so long as the filer/controller so
    // we should not have to copy it.
    p->filterName = fName;
    p->dict = dict;
    ASSERT(p->pName, "strdup() failed");
    p->type = type;
    p->userData = userData;
    p->setCallback = setCallback;
    p->cleanup = cleanup;
    return p;
}


struct QsParameter *
qsParameterCreateForFilter(struct QsFilter *f,
        const char *pName, enum QsParameterType type,
        int (*setCallback)(struct QsParameter *parameter,
            void *value, const char *pName, void *userData),
        void (*cleanup)(const char *pName, void *userData),
        void *userData) {

    DASSERT(f);
    DASSERT(f->name);
    DASSERT(f->parameters);

    // Create parameter dictionary entry.
    struct QsDictionary *d;
    int ret = qsDictionaryInsert(f->parameters, pName, "p", &d);
    ASSERT(ret >= 0);
    if(ret) {
        NOTICE("Parameter %s:%s already exists", f->name, pName);
        return 0;
    }
    DASSERT(d);

    // Create and add the parameter data to this parameter dict
    return CreateParameter(f->parameters, f->stream,
            d, f->name, pName, setCallback,
            cleanup, userData, type);
}

struct QsParameter *
qsParameterCreate(const char *pName, enum QsParameterType type,
        int (*setCallback)(struct QsParameter *parameter,
            void *value, const char *pName, void *userData),
        void (*cleanup)(const char *pName, void *userData),
        void *userData) {

    struct QsFilter *f = GetFilter();
    if(f) {
        // Create this parameter owned by a filter.
        ASSERT(f->mark == _QS_IN_CONSTRUCT, "qsParameterCreate() "
                "must be called in a filter module construct()");

        return qsParameterCreateForFilter(f, pName, type,
                setCallback, cleanup, userData);
    }

    // else: Create this parameter owned by a controller.
    //
    struct QsController *c = pthread_getspecific(_qsControllerKey);
    DASSERT(c);
    DASSERT(c->mark == _QS_IN_CCONSTRUCT ||
            c->mark == _QS_IN_CDESTROY ||
            c->mark == _QS_IN_PRESTART ||
            c->mark == _QS_IN_POSTSTART ||
            c->mark == _QS_IN_PRESTOP ||
            c->mark == _QS_IN_POSTSTOP);

    if(!c) {
        ERROR("qsParameterCreate() cannot find object");
        return 0; // error
    }

    // Create parameter dictionary entry.
    struct QsDictionary *d;
    int ret = qsDictionaryInsert(c->parameters, pName, "p", &d);
    ASSERT(ret >= 0);
    if(ret) {
        ERROR("Parameter %s:%s already exists", c->name, pName);
        return 0;
    }
    DASSERT(d);

    // Create and add the parameter data to this parameter dict
    return CreateParameter(c->parameters, c->app,
            d, c->name, pName, setCallback,
            cleanup, userData, type);
}


static int
AddGetCallback(struct QsParameter *p, const char *filterName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            const void *value,
            void *streamOrApp,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData, uint32_t flags) {
    
    DASSERT(p);
    // p->setCallback is not necessary.  A filter or controller can just
    // set the parameter at flow time by calling qsParameterPush().
    //DASSERT(p->setCallback);

    if(type != Any && p->type != type) {
        ERROR("Parameter \"%s:%s\" type \"%s\" "
                "is not requested type \"%s\"",
                filterName, pName, GetTypeString(p->type),
                GetTypeString(type));
        return -3; // error
    }

    if(flags & QS_KEEP_ONE)
        // Check that we only add this callback just once.
        for(size_t i=p->numGetCallbacks-1; i!=-1; --i)
            if(getCallback == p->getCallbacks[i].getCallback)
                // It's there already, so we're done.
                return 0;


    size_t num = p->numGetCallbacks;

    p->getCallbacks = realloc(p->getCallbacks,
            (num+1)*sizeof(*p->getCallbacks));
    ASSERT(p->getCallbacks, "realloc(,%zu) failed",
            (num+1)*sizeof(*p->getCallbacks));
    struct GetCallback *gc = p->getCallbacks + num;
    gc->getCallback = getCallback;
    gc->userData = userData;
    gc->flags = flags;
    ++p->numGetCallbacks;

    return 1; // success
}


struct Check_AddGetCallback_args {

    enum QsParameterType type;
    const char *filterName;
    int numParameters; // number of parameters added
    regex_t regex; // libc regular expression
    int (*getCallback)(
            const void *value,
            void *streamOrApp,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData);
    void *userData;
    uint32_t flags;
};


static
int Check_AddGetCallback(const char *pName, struct QsParameter *p,
        struct Check_AddGetCallback_args *args) {

    if(args->type != Any && args->type != p->type)
        // It's not the type we are looking for.
        return 0;


    if(regexec(&args->regex, pName, 0, NULL, 0) == 0) {
        // This parameter name matches the regular expression.
        AddGetCallback(p, args->filterName, pName, p->type,
                args->getCallback, args->userData, args->flags);
        ++args->numParameters;
    }
    return 0; // keep going.
}


int qsParameterGet(void *as, const char *filterName,
        const char *pName, enum QsParameterType type,
        int (*getCallback)(
            const void *value,
            void *streamOrApp,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData, uint32_t flags) {

    DASSERT(as);
    DASSERT(filterName);
    DASSERT(filterName[0]);
    DASSERT(pName);
    DASSERT(pName[0]);

    struct QsDictionary *pDict = GetParameterDictionary(as, filterName);
    if(!pDict)
        return -1;

    if(pName && pName[0] && ! (flags & QS_PNAME_REGEX)) {
    
        // Get the parameter from the Dictionary:
        struct QsParameter *p = 0;
        struct QsDictionary *d = qsDictionaryFindDict(pDict,
                pName, (void*) &p);

        if(!d) {
            WARN("Parameter \"%s:%s\" not found", filterName, pName);
            return -2; // error
        }

        return AddGetCallback(p, filterName, pName, type, getCallback,
                userData, flags);
    }

    // This could be getting many parameters via a parameter name regular
    // expression, pName.  We need a struct to pass many arguments in a
    // ForEach callback to iterate through the parameter dictionary
    // with.
    //
    struct Check_AddGetCallback_args args;
    args.type = type;
    args.filterName = filterName;
    args.numParameters = 0;
    args.getCallback = getCallback;
    args.userData = userData;
    args.flags = flags;

    int ret = regcomp(&args.regex, pName, REG_EXTENDED);
    if(ret == REG_ESPACE)
        ASSERT(0, "regcomp(,\"%s\",REG_EXTENDED) failed", pName);
    if(ret) {
        ERROR("Bad regular expression \"%s\"", pName);
        return -3;
    }

    qsDictionaryForEach(pDict,
                (int (*) (const char *key, void *value,
                        void *userData)) Check_AddGetCallback,
                &args);

    regfree(&args.regex);

    return args.numParameters; // success
}


static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
// We put a pointer to the filter or the controller in thread specific
// data with this key.  You see: only filters or controller modules manage
// parameters.
static pthread_key_t parameterKey;

static void MakeKey(void) {
    CHECK(pthread_key_create(&parameterKey, 0));
}



int qsParameterSet(void *sa,
        const char *filterName, const char *pName,
        enum QsParameterType type, void *value) {

    ASSERT("Add controller support to this");

    DASSERT(sa);
    DASSERT(filterName);
    DASSERT(filterName[0]);
    DASSERT(pName);
    DASSERT(pName[0]);
    struct QsParameter *p = 0;

    struct QsFilter *f = 0;
    struct QsController *c = 0;

    struct QsStream *s = sa;
    if(s->type == _QS_STREAM_TYPE) {

        f = qsFilterFromName(s, filterName);
        if(!f) {
            WARN("Filter named \"%s\" not found", filterName);
            return 1; // error
        }
        DASSERT(f->parameters);

        // Get the parameter from the Dictionary:
        p = qsDictionaryFind(f->parameters, pName);
        if(!p) {
            WARN("Parameter \"%s:%s\" not found", filterName, pName);
            return 2; // error
        }
    } else {

        struct QsApp *app = sa;
        DASSERT(app->type == _QS_APP_TYPE);
        c = qsDictionaryFind(app->controllers,
                filterName);
        if(!c) {
            WARN("Controller named \"%s\" not found", filterName);
            return 3; // error
        }
        DASSERT(c->parameters);

        // Get the parameter:
        p = qsDictionaryFind(c->parameters, pName);
        if(!p) {
            WARN("Parameter \"%s:%s\" not found", filterName, pName);
            return 4; // error
        }
    }

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

    void *oldOwner = pthread_getspecific(parameterKey);

    // Set the thread specific
    CHECK(pthread_setspecific(parameterKey, (f)?((void *)f):((void*)c)));

    // This may call qsParameterPush() or it may not, or qsParameterPush()
    // may be called later, after this call.  It's up to the filter module
    // when and if to call qsParameterPush().  It may not call it if the
    // parameter does not change due to this call.
    p->setCallback(p, value, pName, p->userData);

    CHECK(pthread_setspecific(parameterKey, oldOwner));

    // Now we wait for this to have an effect.  The effect does not have
    // to be soon.

    return 0; // success
}


int qsParameterPushByPointer(const struct QsParameter *p,
        void *value) {

    DASSERT(p);
    // p->setCallback is not necessary.
    //DASSERT(p->setCallback);
    DASSERT(p->filterName);
    DASSERT(p->filterName[0]);

    struct GetCallback *gcs = p->getCallbacks;
    size_t num = p->numGetCallbacks;
    for(size_t i=0; i<num; ++i)
        gcs[i].getCallback(value, p->streamOrApp,
                p->filterName, p->pName, p->type, gcs[i].userData);

    return 0;
}


// This is call inside the filter setCallback()
//
// This is the effect we where waiting for from the filter module
// the owns the parameter.
//
int qsParameterPush(const char *pName, void *value) {

    // First get the filter pointer some how.

    // This function must be called from the filter or controller module
    // somewhere:
    //
    // So it may be called from the filter (controller) setCallback() from
    // a qsParameterSet() and if so this will get the filter (controller)
    // object:
    
    CHECK(pthread_once(&keyOnce, MakeKey));
    struct QsParameter *p = 0;


    // First we try to find a filter or controller that is calling
    // qsParameterSet().
    //
    // Here's the thing: the thread specific data may be a filter or a
    // controller, but it does not matter which we use because they both
    // inherit struct QsDictionary as variable named parameters.
    //
    struct QsFilter *f = pthread_getspecific(parameterKey);
    if(f) {
 
        // NOTE: The only data we can access in f is f->parameters.
        // Because we are accessing a sub class of this struct
        // QsFilter.

        // Get the parameter from the Dictionary:
        p = qsDictionaryFind(f->parameters, pName);
        if(!p) {
            WARN("Parameter \"%s\" not found", pName);
            return 2; // error
        }
        // We should have p now.
    }

    if(!f)
        // Or it may be called after the filter setCallback() in one of
        // the filter module functions start() stop() or input().
        // GetFilter() is a small wrapper that uses a different pthread
        // specific variable.
        f = GetFilter();

    if(f) {
        // Get the parameter from the Dictionary:
        p = qsDictionaryFind(f->parameters, pName);
        if(!p) {
            WARN("qsParameterPush(\"%s\") calling thread not found",
                    pName);
            return 3; // error
        }
        // We should have p now.
    } else {

        // Now try to find a controller using the controller stuff
        // and get the dictionary and parameter from it.
        //
        struct QsController *c = pthread_getspecific(_qsControllerKey);
        if(!c) {
            WARN("Parameter \"%s\" not found", pName);
            return 4; // error
        }

        DASSERT(c);
        DASSERT(c->mark == _QS_IN_CCONSTRUCT ||
            c->mark == _QS_IN_CDESTROY ||
            c->mark == _QS_IN_PRESTART ||
            c->mark == _QS_IN_POSTSTART ||
            c->mark == _QS_IN_PRESTOP ||
            c->mark == _QS_IN_POSTSTOP);

        // Get the parameter from the Dictionary:
        p = qsDictionaryFind(c->parameters, pName);
        if(!p) {
            WARN("Parameter \"%s:%s\" not found", c->name, pName);
            return 3; // error
        }
    }

    return qsParameterPushByPointer(p, value);
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
    // Kind of a waste of memory given regex, but regex_t does not have a
    // null like value.
    bool useRegex;
    regex_t regex;
};


static
int ParameterForEach(const char *key, void *value,
        struct ParameterCBArgs *cbArgs) {

    struct QsFilter *f = cbArgs->filter;
    const struct QsParameter *p = value;
    DASSERT(f);

    int ret = 0;

    if((void *) f != value &&
            (!cbArgs->type || cbArgs->type == p->type) &&
            ( cbArgs->useRegex &&
                regexec(&cbArgs->regex, key, 0, NULL, 0) == 0)
            ) {
        ret = cbArgs->callback(f->stream, f->name, p->pName,
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
        void *userData, bool *done, uint32_t flags) {

    DASSERT(f->parameters);

    if(pName && pName[0] && !(flags & QS_PNAME_REGEX)) {
        struct QsParameter *p = qsDictionaryFind(f->parameters, pName);
        if(!p) {
            WARN("Parameter \"%s:%s\" not found", f->name, pName);
            return 0;
        }
        if(type != Any || type == p->type) {
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
        done,
        false
    };

    if(pName && pName[0] && (flags & QS_PNAME_REGEX)) {
        int ret = regcomp(&args.regex, pName, REG_EXTENDED);
        if(ret == REG_ESPACE)
            ASSERT(0, "regcomp(,\"%s\",REG_EXTENDED) failed", pName);
        if(ret) {
            ERROR("Bad regular expression \"%s\"", pName);
            return 0;
        }
        args.useRegex = true;
    }

    // Subtract 1 for the call with the filter.
    size_t ret = qsDictionaryForEach(f->parameters, 
            (int (*)(const char *, void *, void *))
            ParameterForEach, &args) - 1;

    if(args.useRegex)
        regfree(&args.regex);

    return ret;
}


static size_t
ForStreamParameters(struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData, bool *done, uint32_t flags) {

    DASSERT(s);

    if(!filterName) {
        // We go through all filters in this stream.
        size_t ret = 0;
        for(struct QsFilter *f = s->filters; f; f = f->next) {
            ret += ForParameterDict(f, pName, type,
                    callback, userData, done, flags);
            if(*done) break;
        }
        return ret;
    }

    struct QsFilter *f = qsFilterFromName(s, filterName);

    // Just this one filter.
    return ForParameterDict(f, pName, type, callback, userData, done, flags);
}


size_t qsParameterForEach(struct QsApp *app, struct QsStream *s,
        const char *filterName, const char *pName,
        enum QsParameterType type,
        int (*callback)(
            struct QsStream *stream,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData),
        void *userData, uint32_t flags) {

    bool done = false;

    if(!s) {
        size_t ret = 0;
        ASSERT(app, "app must be set to call qsParameterForEach() "
                "if stream is not set");
        // We go through all streams in this app.
        for(s = app->streams; s; s = s->next) {
            ret += ForStreamParameters(s,
                    filterName, pName, type,
                    callback, userData, &done, flags);
            if(done) break;
        }
        return ret;
    }
    // Just this one stream.
    return ForStreamParameters(s, filterName,
            pName, type, callback, userData, &done, flags);
}
