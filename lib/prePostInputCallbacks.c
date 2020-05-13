#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdatomic.h>


// The public installed user interfaces:
#include "../include/quickstream/app.h"
#include "../include/quickstream/controller.h"


// Private interfaces.
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "controllerCallbacks.h"


static void CleanUpCB(void *ptr) {
    DASSERT(ptr);
#ifdef DEBUG
    memset(ptr, 0, sizeof(struct ControllerCallback));
#endif
    free(ptr);
}

#if 0

int qsAddPreFilterInput(struct QsFilter *f,
        int (*callback)(
            struct QsFilter *filter,
            const size_t len[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData), void *userData) {

    DASSERT(f);
    struct QsController *c = pthread_getspecific(_qsControllerKey);
    ASSERT(c);
    DASSERT(c->name);
    DASSERT(c->name[0]);
    DASSERT(c->mark == _QS_IN_CCONSTRUCT ||
            c->mark == _QS_IN_CDESTROY ||
            c->mark == _QS_IN_PRESTART ||
            c->mark == _QS_IN_POSTSTART ||
            c->mark == _QS_IN_PRESTOP ||
            c->mark == _QS_IN_POSTSTOP);
    DASSERT(f->stream);
    DASSERT(f->stream->app);
    ASSERT(f->stream->app == c->app, "filter \"%s\" is from a"
            " different app than controller \"%s\"",
            f->name, c->name);

    if(!f->preInputCallbacks)
        f->preInputCallbacks = qsDictionaryCreate();

    struct ControllerCallback *cb = malloc(sizeof(*cb));
    ASSERT(cb, "malloc(%zu) failed", sizeof(*cb));

    struct QsDictionary *d = 0;
    int ret = qsDictionaryInsert(f->preInputCallbacks, c->name, cb, &d);
    ASSERT(ret >= 0, "Bad controller name \"%s\"", c->name);
    DASSERT(d);

    if(ret) {
        free(cb);
        cb = qsDictionaryGetValue(d);
        INFO("Replaced PreInput Callback for filter:controller="
                "\"%s:%s\"", f->name, c->name);
    } else {
        DASSERT(ret == 0);
        qsDictionarySetFreeValueOnDestroy(d, CleanUpCB);
        DSPEW("Added PreInput Callback for filter:controller="
                "\"%s:%s\"", f->name, c->name);
    }

    cb->callback = callback;
    cb->userData = userData;
    cb->returnValue = 0;

    return 0; // success
}

#endif


int qsAddPostFilterInput(struct QsFilter *f,
        int (*callback)(
            struct QsFilter *filter,
            const size_t lenIn[],
            const size_t lenOut[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData), void *userData) {

    DASSERT(f);
    struct QsController *c = pthread_getspecific(_qsControllerKey);
    ASSERT(c);
    DASSERT(c->name);
    DASSERT(c->name[0]);
    DASSERT(c->mark == _QS_IN_CCONSTRUCT ||
            c->mark == _QS_IN_CDESTROY ||
            c->mark == _QS_IN_PRESTART ||
            c->mark == _QS_IN_POSTSTART ||
            c->mark == _QS_IN_PRESTOP ||
            c->mark == _QS_IN_POSTSTOP);
    DASSERT(f->stream);
    DASSERT(f->stream->app);
    ASSERT(f->stream->app == c->app, "filter \"%s\" is from a"
            " different app than controller \"%s\"",
            f->name, c->name);

    if(!f->postInputCallbacks)
        f->postInputCallbacks = qsDictionaryCreate();

    struct ControllerCallback *cb = malloc(sizeof(*cb));
    ASSERT(cb, "malloc(%zu) failed", sizeof(*cb));

    struct QsDictionary *d = 0;
    int ret = qsDictionaryInsert(f->postInputCallbacks, c->name, cb, &d);
    ASSERT(ret >= 0, "Bad controller name \"%s\"", c->name);
    DASSERT(d);

    if(ret) {
        free(cb);
        cb = qsDictionaryGetValue(d);
        INFO("Replaced PostInput Callback for filter:controller="
                "\"%s:%s\"", f->name, c->name);
    } else {
        DASSERT(ret == 0);
        qsDictionarySetFreeValueOnDestroy(d, CleanUpCB);
        DSPEW("Added PostInput Callback for filter:controller="
                "\"%s:%s\"", f->name, c->name);
    }

    cb->callback = callback;
    cb->userData = userData;
    cb->returnValue = 0;

    return 0; // success
}
