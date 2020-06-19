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

// The public installed user interfaces:
#include "../../../../include/quickstream/app.h"

// Private interfaces.
#include "../../../debug.h"
#include "../../../Dictionary.h"
#include "../../../qs.h"
#include "../../../LoadDSOFromTmpFile.h"

// This file provides the scriptControllerLoader interface
// which we define in this header file:
#include "scriptControllerLoader.h"
#include <Python.h>



int initialize(void) {

    WARN();
    Py_Initialize();

    return 0; // success
}

// TODO: Can this get called more than once?
//
// This need to return the handle from dlopen() of a controller module
// that is pythonController.so.  pythonController.so will wrap the
// python script file in path.
//
void *loadControllerScript(const char *path, int argc, const char **argv) {

    WARN("Loading controller python module \"%s\"", path);

    

    return 0; // fail
}


void cleanup(void) {

    Py_FinalizeEx();
    WARN();
}
