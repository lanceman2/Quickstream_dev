
// Used in controller loading code to see if a copy of the controller DSO
// needs to be reloaded, so that we don't dlopen() the same DSO twice.
//
// This will dlclose() *dlhandle if it is not unique, and return a new
// handle as a replacement.  *dlhandle must be a valid value from dlopen().
//
// Returns handle on success, else returns 0 if a unique handle cannot be
// returned.
//
extern
void *
GetUniqueControllerHandle(struct QsApp *app, void **dlhandle,
        const char *path);
