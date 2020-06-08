#ifndef __qsapp_h__
#  error "app.h needs to be included before this file."
#endif

#ifndef __debug_h__
#  error "debug.h needs to be included before this file."
#endif


static inline
struct QsFilter *FindFilterNamed(struct QsApp *app, const char *name) {


    for(struct QsStream *s = app->streams; s; s = s->next) {
        struct QsFilter *F = s->filters;
        // TODO: This could be made quicker, but the quickstream is not in
        // "run" mode now so speed it not really needed now.
        //
        for(F = s->filters; F; F = F->next) {
            if(!strcmp(F->name, name))
                return F; // found
        }
    }
    return 0; // not found
}


// Here is where we cleanup filter (QsFilter) data that is from loading
// and not part of flow-time and stream resources.
//
// Flow-time and stream resources are like ring buffers and connections.
//
// This should not be called from another file.
//
static inline
void FreeFilter(struct QsFilter *f) {

    DASSERT(f);
    DASSERT(f->name);
    DASSERT(f->outputs == 0);
    DASSERT(f->numOutputs == 0);
    DASSERT(f->app);
    DASSERT(f->stream);


    if(f->dlhandle) {
        int (* destroy)(void) = dlsym(f->dlhandle, "destroy");
        if(destroy) {

            struct QsFilter *oldFilter = pthread_getspecific(_qsKey);
            // Check if the user is using more than one QsApp in a single
            // thread and calling qsApp or qsStream functions from within
            // other qsApp or qsStream functions.
            //
            // You may be able to create more than one QsApp but:
            //
            ASSERT(!oldFilter || oldFilter->app == f->app,
                    "You cannot mix QsApp objects in "
                    "other QsApp function calls");

            DASSERT(f->mark == 0);

            CHECK(pthread_setspecific(_qsKey, f));

            // This FreeFilter() function must be re-entrant.  It may be
            // called from a qsFilterUnload() call in a filter->destroy()
            // call; for filters that are super-modules.  Super-modules
            // are filters that can load and unload other filters.
            //
            // However, filters cannot unload themselves.
            //
            // A filter cannot unload itself.
            DASSERT(oldFilter != f);
            // Use the mark variable at a state marker.
            f->mark = _QS_IN_DESTROY;

            int ret = destroy();

            // Filter f is not in destroy() phase anymore.
            f->mark = 0;

            CHECK(pthread_setspecific(_qsKey, oldFilter));


            if(ret) {
                // TODO: what do we use this return value for??
                //
                // Maybe just to print this warning.
                WARN("filter \"%s\" destroy() returned %d", f->name, ret);
            }
        }

        dlerror(); // clear error
        if(dlclose(f->dlhandle))
            WARN("dlclose(%p): %s", f->dlhandle, dlerror());
            // TODO: So what can I do.
    }

    if(f->preInputCallbacks)
        qsDictionaryDestroy(f->preInputCallbacks);
    if(f->postInputCallbacks)
        qsDictionaryDestroy(f->postInputCallbacks);


#ifdef DEBUG
    memset(f->name, 0, strlen(f->name));
#endif
    free(f->name);
    memset(f, 0, sizeof(*f));
    free(f);
}


static inline
struct QsFilter *FindFilter_viaHandle(struct QsStream *s, void *handle) {
    DASSERT(s);
    DASSERT(handle);

    for(struct QsFilter *f = s->filters; f; f = f->next)
        if(f->dlhandle == handle)
            return f;
    return 0;
}


//
// Free the filter memory, dlcose() the handle, and remove it from the
// stream lists.
//
static inline
void DestroyFilter(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s);
    DASSERT(f);
    DASSERT(s->filters);
    DASSERT(f->stream);
    DASSERT(s == f->stream);
    DASSERT(s->dict);
    DASSERT(f->name);

    DSPEW("Freeing: %s", f->name);
    
    if(f->parameters)
        // This will cleanup all the parameter data using the qsDictionary
        // SetFreeValueOnDestroy thingy.
        qsDictionaryDestroy(f->parameters);

    ASSERT(0 == qsDictionaryRemove(s->dict, f->name),
            "Can't remove filter \"%s\" from source dict", f->name);

    // Remove this filter from any stream connections if there are any.
    StreamRemoveFilterConnections(s, f);

    // Remove it from the stream list.
    struct QsFilter *F = s->filters;
    struct QsFilter *prev = 0;
    while(F) {
        if(F == f) {
            if(prev)
                prev->next = F->next;
            else
                s->filters = F->next;

            FreeFilter(f);
            break;
        }
        prev = F;
        F = F->next;
    }

    DASSERT(F, "Filter was not found in streams filter list");
}



// name must be unique for all filters in stream
//
// TODO: this is not required to be fast; yet.
//
static inline
struct QsFilter *AllocAndAddToFilterList(struct QsStream *s,
        const char *name) {

    DASSERT(s);
    DASSERT(name);
    DASSERT(name[0]);
    struct QsFilter *f = calloc(1, sizeof(*f));
    ASSERT(f, "calloc(1, %zu) failed", sizeof(*f));

    // Check for unique name for this loaded module filter.
    //
    if(FindFilterNamed(s->app, name)) {
        uint32_t count = 2;
        size_t sLen = strlen(name) + 7;
        f->name = malloc(sLen);

#define MAX  10000000

        while(count < MAX) {
            snprintf(f->name, sLen, "%s-%" PRIu32, name, count);
            ++count;
            if(!FindFilterNamed(s->app, f->name))
                break;
        }
        // I can't imagine that there will be ~ 10000000 filters.
        DASSERT(count < MAX);
    } else
        f->name = strdup(name);


    struct QsFilter *fIt = s->filters; // dummy iterator.
    if(!fIt)
        // This is the first filter in this stream.
        return (s->filters = f);

    while(fIt->next) fIt = fIt->next;

    // Put it last in the stream filter list.
    fIt->next = f;

    return f;
}
