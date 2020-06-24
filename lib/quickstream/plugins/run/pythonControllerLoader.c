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
#define PY_SSIZE_T_CLEAN
#include <Python.h>


// Returns malloc allocated memory that must be free()ed.
static char *
GetInstalledControllerPath(const char *suffix) {

    // postLen = strlen("/lib/quickstream/plugins/controllers")
    const ssize_t postLen = strlen(suffix) + 1;
    DASSERT(postLen);
    DASSERT(postLen < 1024*1024);

    ssize_t bufLen = 128 + postLen;
    char *buf = (char *) malloc(bufLen);
    ASSERT(buf, "malloc(%zu) failed", bufLen);
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    while(rl + postLen >= bufLen)
    {
        DASSERT(bufLen < 1024*1024); // it should not get this large.
        buf = realloc(buf, bufLen += 128);
        ASSERT(buf, "realloc() failed");
        rl = readlink("/proc/self/exe", buf, bufLen);
        ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    }

    buf[rl] = '\0';
    //
    // Now buf = path to program binary.
    //
    // Now strip off to after "/bin/qs_runner" (Linux path separator)
    // Or "/testing_crap/crap_runner"
    // by backing up two '/' chars.
    //
    --rl;
    while(rl > 5 && buf[rl] != '/') --rl; // one '/'
    ASSERT(buf[rl] == '/');
    --rl;
    while(rl > 5 && buf[rl] != '/') --rl; // two '/'
    ASSERT(buf[rl] == '/');
    buf[rl] = '\0'; // null terminate string
    // Now add the suffix.
    // So long as we counted correctly strcat() is safe.
    strcat(&buf[rl], suffix);

    return buf;
}


static void
SetEnvPythonPath(void) {

    // Python 3.8.2 BUG: Py_Initialize() and Py_InitializeEx() require the
    // PYTHONPATH environment variable to be set or we cannot run any
    // python script from C.  It will not even let us use the full path of
    // the python script file.  We figured this out the hard way by trial
    // and error.

    // We concatenate env variables in this order: ., $QS_MODULE_PATH,
    // $QS_CONTROLLER_PATH, $PYTHONPATH, and then
    // PATH_TO_THE_RUNNING_PROGRAM/../lib/quickstream/plugins/controllers
    // where PATH_TO_THE_RUNNING_PROGRAM comes from
    // readlink(\"/proc/self/exe\",,); we put all them together with a ';'
    // separator in PYTHONPATH.
    //
    // TODO: Given this python BUG, if the full path of the python script
    // is given this is not likely to work.

    char *modEnv = getenv("QS_MODULE_PATH");
    char *conEnv = getenv("QS_CONTROLLER_PATH");
    char *pyEnv = getenv("PYTHONPATH");
    char *instPath = GetInstalledControllerPath(
            "/lib/quickstream/plugins/controllers");

    size_t len = 2;
    if(modEnv) len += strlen(modEnv) + 1;
    if(conEnv) len += strlen(conEnv) + 1;
    if(pyEnv) len += strlen(pyEnv) + 1;
    len += strlen(instPath) + 1;

    char *pyPath = malloc(len);
    char *mem = pyPath;
    ASSERT(pyPath, "malloc(%zu) failed", len);

    *(pyPath++) = '.';

    if(modEnv)
        pyPath += sprintf(pyPath, ":%s", modEnv);
    if(conEnv)
        pyPath += sprintf(pyPath, ":%s", conEnv);
     if(pyEnv)
        pyPath += sprintf(pyPath, ":%s", pyEnv);
    sprintf(pyPath, ":%s", instPath);

    ASSERT(setenv("PYTHONPATH", mem, 1) == 0);

    WARN("PYTHONPATH=\"%s\"", mem);

    free(mem);
    free(instPath);
}


// This gets called once.
int initialize(void) {

    SetEnvPythonPath();

    //size_t l = 0;
    //wchar_t *programName = Py_DecodeLocale("quickstream", &l);
    //Py_SetProgramName(programName);
    // Is this supposed to be calling now.
    //PyMem_RawFree(programName);
    
    // Py_Initialize() will not work, we need to keep python from
    // catching signals, and Py_InitializeEx(0) does that.
    Py_InitializeEx(0);

    PyEval_InitThreads();

    return 0; // success
}


// This get called more than once.
//
// This need to return the handle from dlopen() of a controller module
// that is pythonController.so.  pythonController.so will wrap the
// python script file in path.
//
void *loadControllerScript(const char *pyPath, int argc,
        const char **argv) {

    DASSERT(pyPath);
    DASSERT(pyPath[0]);

    DSPEW("Loading controller python module \"%s\"", pyPath);

    // We open a C DSO python wrapper.
    char *dsoPath = GetPluginPath(MOD_PREFIX, "run/",
            "pythonController", ".so");
WARN("dsoPath=\"%s\"", dsoPath);
    void *dlhandle = dlopen(dsoPath, RTLD_NOW | RTLD_LOCAL);

WARN();

    if(!dlhandle) {
        ERROR("dlopen(\"%s\",) failed: %s", dsoPath, dlerror());
        goto fail;
    }

    WARN("loaded \"%s\"", dsoPath);

    // Call pyInit(pyPath) so it may get the python script loaded and
    // ready.
    dlerror(); // clear error
    int (*pyInit)(const char *) = dlsym(dlhandle, "pyInit");
    char *err = dlerror();
    if(err) {
        ERROR("dlsym(,\"pyInit\") failed: %s", err);
        goto fail;
    }

    if(pyInit(pyPath))
        goto fail;

    free(dsoPath);

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

    WARN();

    // TODO: this crashes the program with stderr spew:
    // free(): invalid pointer
    
    Py_FinalizeEx();
    
    // We don't get past the above call.

    WARN();
}
