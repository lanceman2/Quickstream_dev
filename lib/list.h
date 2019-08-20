// It would appear that all (most) the implementations of things like red
// black trees, singly and doubly linked lists, and other generic lists,
// use an allocation that is separate from the allocation of the element
// that is being stored.  So to make and add and element you must do two
// separate allocations one for the element and one for the list pointers.
// That seems wasteful to me.  If the element being stored has the list
// data structure in it, then we can add an element with just one
// allocation.  This is an obvious optimization.  We also get one less
// layer of pointer indirection when we get the element from the list, and
// we get the element typed at compile time, and do not access another
// pointer.
//
// For lists in quickstream, we insist that the access to the list be
// quick when the stream is in a "run" state, and the lists are not edited
// while it is in a "run" state.
//
// Arrays may be the quickest list data structure that there is, and
// quicker than a hash table, given that we do not calculate a hash
// function to index into the table.


//
// TODO: Maybe make this into a red black tree with the same interfaces.
// They maybe needed when lookup by name string is needed by (filter API)
// user code.
//


#ifndef __qsapp_h__
#  error "qsapp.h needs to be included before this file."
#endif
#ifndef __debug_h__
#  error "qsapp.h needs to be included before this file."
#endif

static inline struct QsFilter *FindFilterName(struct QsApp *app,
        const char *name) {

    struct QsFilter *F = app->filters;
    // TODO: This could be made quicker, but the quickstream is
    // not in "run" mode now so speed it not really needed now.
    //
    for(F = app->filters; F; F = F->next) {
        if(!strcmp(F->name, name))
            return F; // found
    }

    return 0; // not found
}



// name must be unique for all filters in app
//
// TODO: this is not required to be fast; yet.
//
static inline struct QsFilter *AllocAndAddToFilterList(struct QsApp *app,
        const char *name) {

    DASSERT(app, "");
    struct QsFilter *f = calloc(1, sizeof(*f));
    ASSERT(f, "calloc(1, %zu) failed", sizeof(*f));
    f->name = strdup(name);


    struct QsFilter *F = app->filters;
    if(!F) {
        // This is the first filter in this app.
        app->filters = f;
        return f;
    }
    while(F->next) F = F->next;
    F->next = f;
    DASSERT(0 == f->next, "");

    /* TODO: Check for unique name for this loaded module filter. */
    //
    // TODO: This could be made quicker, but the quickstream is
    // not in "run" mode now so speed is not really needed now.

    return f;
}

