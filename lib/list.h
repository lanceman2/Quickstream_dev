// It would appear that all the implementations of things like red black
// trees, singly and doubly linked lists, and other generic lists, use an
// allocation that is separate from the allocation of the element that is
// being stored.  So to make and add and element you must do two
// separate allocations one for the element and one for the list pointers.
// That seems wasteful to me.  If the element being stored has the list
// data structure in it, then we can add an element with just one
// allocation.  WTF, this is obvious.  We also get one less layer of
// pointer indirection when we get the element from the list, and we get
// the element typed at compile time, and do not access another pointer.

//
// TODO: Maybe make this into a red black tree with the same interfaces.
//






static inline void addFilterToList(struct Fs *fs, struct Filter *f) {

    struct Filter *lastF = fs->filters;
    if(!lastF) {
        fs->filters = f;
        return;
    }
    while(lastF->next) lastF = lastF->next;
    lastF->next = f;
    DASSERT(0 == f->next, "");
}


