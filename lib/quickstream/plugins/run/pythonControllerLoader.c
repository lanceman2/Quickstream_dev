/* We make this file (and maybe others) into a DSO (dynamic shared object)
 * plugin, so that the program quickstream and libquickstream.so do not
 * require python and the python libraries to build or run.  The use of
 * python in quickstream is optional.  The linking to (for example)
 * libpython3.8.so is done on the fly.  We could run quickstream and while
 * it is running, install python, and then the already running quickstream
 * program could use that new python installation and run python scripts
 * by loading this DSO file.
 *
 * quickstream is not married to python.
 *
 * 'ldd quickstream' does not list libpython,
 *
 * We take this idea further, this file loads python, and the core of
 * quickstream knows nothing about python.  Just the python module loaders
 * (like this one) know anything about python and the python C API.
 *
 *
 *
 * The only thing the core of quickstream know about the strings:
 *
 * filename: pythonControllerLoader
 *
 * script suffix ".py"
 *
 *
 * TODO: How do the compiled .pyc files come about?
 * Can we use them?  Are they always in the same directory
 * as the .py files?
 *
 *
 *************************************************************************
 *
 *  Why do we have pythonControllerLoader.so and pythonController.so?
 *
 *************************************************************************
 *
 *  We needed a controller from pythonController.so for each Python module
 *  that is loaded.   Each loaded pythonController.so has it's own C state
 *  data that is associated with the python file that it loads.  This
 *  makes each loaded pythonController.so run independently with different
 *  C global data and that's simpler than code that keeps lists of the
 *  loaded objects.  The plugin DSO lists is already taken care of in
 *  QsApp.  We do not need to write that functionality again.   The cost
 *  is loading another module DSO file, but the total code written is
 *  less; and it could be argued that it is less complex.
 *
 *  We needed there to be one code that initializes the python
 *  interrupter, that being pythonControllerLoader.so.
 *  pythonControllerLoader.so is just one version of a controller script
 *  loader.   We plan to make other controller script loaders, like for
 *  example one that can load LUA.  LUA may be better than python for this
 *  kind of stuff: https://www.lua.org/  Lua is a lightweight, high-level,
 *  multi-paradigm programming language designed primarily for embedded
 *  use in applications.  We don't think that python is as lightweight as
 *  LUA, and python was not initially designed for embedding like LUA.
 *
 *************************************************************************
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>


// The public installed user interfaces:
#include "../../../../include/quickstream/app.h"

// Private interfaces.
#include "../../../debug.h"
#include "../../../Dictionary.h"
#include "../../../qs.h"
#include "../../../LoadDSOFromTmpFile.h"
#include "../../../controller.h"


// This file provides the scriptControllerLoader interface
// which we define in this header file:
#include "scriptControllerLoader.h"


// List of Python controller modules loaded
static
struct ModuleList *moduleList = 0;


// This gets called once pre quickstream process.
int initialize(void) {

    size_t l = 0;
    wchar_t *programName = Py_DecodeLocale("quickstream", &l);
    Py_SetProgramName(programName);
    PyMem_RawFree(programName);

    // Py_Initialize() will not work, we need to keep python from
    // catching signals, and Py_InitializeEx(0) does that.
    Py_InitializeEx(0);

    return 0; // success
}


// This gets called more than once.
//
// This need to return the handle from dlopen() of a controller module
// that is pythonController.so.  pythonController.so will wrap the
// python script file in path.
//
void *loadControllerScript(const char *pyPath, struct QsApp *app,
        struct QsController *c) {

    DASSERT(pyPath);
    DASSERT(pyPath[0]);

    DSPEW("Loading controller python module \"%s\"", pyPath);

    // We open a C DSO python wrapper.
    char *dsoPath = GetPluginPath(MOD_PREFIX, "run/",
            "pythonController", ".so");
    void *dlhandle = dlopen(dsoPath, RTLD_NOW | RTLD_LOCAL);

    if(dlhandle && 
            // This will load a tmp copy if it has too, to make it a
            // unique dlhandle.  dlopen() will return the same handle if
            // it loads the same file again.
            !(dlhandle = GetUniqueControllerHandle(app,
                    &dlhandle, dsoPath))) {
        ERROR("dlopen(\"%s\",) failed: %s", dsoPath, dlerror());
        goto fail;
    }

    INFO("loaded \"%s\"", dsoPath);

    // Call pyInit(pyPath, dict) so it may get the python script loaded
    // and ready.
    dlerror(); // clear error
    int (*pyInit)(const char *, struct ModuleList **, const char *,
            struct QsController *) = dlsym(dlhandle, "pyInit");
    char *err = dlerror();
    if(err) {
        ERROR("dlsym(,\"pyInit\") failed: %s", err);
        goto fail;
    }

#define DIR_CHAR  '/'

    // We borrow that dsoPath to use as the DSO directory so we may tell
    // the controller DSO what directory it is in.
    char *s = dsoPath + strlen(dsoPath);
    DASSERT(*s == '\0');
    --s;
    while(s != dsoPath && *s != DIR_CHAR) --s;
    DASSERT(*s == DIR_CHAR);
    *s = '\0';
    // dsoPath now is just the directory.

    if(pyInit(pyPath, &moduleList, dsoPath, c))
        goto fail;

    free(dsoPath);

    // This returned handle will be the controller handle that with have
    // the standard controller functions: construct(), destroy(),
    // preStart(), postStart(), preStop(), postStop(), and help().
    //
    // The python module has these functions as an option.
    // 
    // This returned handle the loaded DSO is just like all the other
    // controllers DSO plugins.  The difference is that there can be only
    // one pythonControllerLoader.so that is loaded, because there can
    // only be one Python interpreter.  That's just our simple design.
    //
    //
    //     * one interpreter loader: pythonControllerLoader.so, and
    //
    //     * one controller loader for each python controller:
    //       pythonController.so.  It's replicated each time it loads
    //       a python script.
    //
    //
    return dlhandle;

fail:
    if(dlhandle)
        dlclose(dlhandle);
    free(dsoPath);
    return 0; // error
}


// This gets called once.
// This gets called after all the python scripts have finished and
// cleaned-up.
void cleanup(void) {

    // valgrind shows libpython3 to have a lot of memory leaks.  Good
    // thing we'll likely be exiting the program soon after this.
    //
    Py_FinalizeEx();

    DASSERT(moduleList == 0,
            "All the python controller modules "
            "have not been unloaded");

    INFO();
}


