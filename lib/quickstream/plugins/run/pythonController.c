// quickstream controller module that add a byte counter for every filter
// input and output port.
//
// This module should be able to work with more than one stream running.


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

#include <Python.h>


#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../debug.h"
#include "../../../qs.h"

// pythonController.h declares interface functions that the
// pythonControllerLoader will use that we define in this file;
// in addition to the common controller interface functions.
#include "pythonController.h"


static PyObject *pModule = 0;

#define DIR_SEP '/'


// We assume that the form of p is like:
// 
//   /home/joe/module/foo.py
//
// Makes:
//   p = "/home/joe/module"
//   returns "foo"
//
static inline
char *base_dirname(char *p) {

    DASSERT(p);
    DASSERT(p[0] == DIR_SEP);

    size_t l = strlen(p);
    DASSERT(l > 3);
    char *s = p + l -1;

    DASSERT(*s == 'y');
    DASSERT(*(s-1) == 'p');
    DASSERT(*(s-2) == '.');
    s -= 2;
    *s = '\0'; // truncate string at '.'
    --s;
    while(s != p && *s != DIR_SEP)
        --s;
    DASSERT(*s == DIR_SEP);
    DASSERT(s != p);
    *s = '\0';
    return s + 1;
}


static inline
PyObject *
GetPyModule(char *path_in) {

    char *dir = strdup(path_in);
    char *modFile = base_dirname(dir);

    // Pain in the ass wide char crap.
    wchar_t *path = Py_DecodeLocale(dir, 0);
    PySys_SetPath(path);
    Py_DECREF(path);
    PyObject *pName = PyUnicode_DecodeFSDefault(modFile);
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    free(dir);
    return pModule;
}


static
PyObject *LoadTmpCopy(const char *path) {

    INFO("Python script from %s was loaded before, "
            "loading a copy", path);
    char tmpFilename[63];
    strcpy(tmpFilename, "/tmp/qs_XXXXXX.py");
    int tmpFd = mkstemps(tmpFilename, 3);
    if(tmpFd < 0) {
        ERROR("mkstemp() failed");
        return 0;
    }
    DSPEW("made temporary file: %s", tmpFilename);
    int rfd = open(path, O_RDONLY);
    if(rfd < 0) {
        ERROR("open(\"%s\", O_RDONLY) failed", path);
        close(tmpFd);
        unlink(tmpFilename);
        return 0;
    }
    const size_t len = 1024;
    uint8_t buf[len];
    ssize_t rr = read(rfd, buf, len);
    while(rr > 0) {
        ssize_t wr;
        size_t bw = 0;
        while(rr > 0) {
            wr = write(tmpFd, &buf[bw], rr);
            if(wr < 1) {
                ERROR("Failed to write to %s", tmpFilename);
                close(tmpFd);
                close(rfd);
                unlink(tmpFilename);
                return 0;
            }
            rr -= wr;
            bw += wr;
        }
 
        rr = read(rfd, buf, len);
    }
    close(tmpFd);
    close(rfd);
    chmod(tmpFilename, 0600);

    pModule = GetPyModule(tmpFilename);

    if(unlink(tmpFilename))
        // There is no big reason to fuss to much.
        WARN("unlink(\"%s\") failed", tmpFilename);

    return pModule;
}


// Note: Only the main thread will access this pointer.
//
static
struct ModuleList **moduleList = 0;


// This is a C quickstream controller module with one extra method,
// pyInit().  We could have gotten the python module file from the
// construct() arguments, but then this would have the interface of a
// regular quickstream plugin DSO and this is not a regular quickstream
// plugin DSO.  This can't work as a regular quickstream plugin DSO, since
// the Python interpreter must be initialize before calling this.
//
// We don't have this DSO plugin initialize the Python interpreter so that
// this DSO is closer to being a regular controller DSO with separate
// global C data, that can be loaded many times as temporary copies.
//
int pyInit(const char *moduleName, struct ModuleList **moduleList_in) {

    moduleList = moduleList_in;

    WARN("moduleName=\"%s\"", moduleName);
    char *modPath = GetPluginPath(MOD_PREFIX, "controllers/",
            moduleName, ".py");

    pModule = GetPyModule(modPath);

    if(pModule) {
        // See if we loaded this module already.
        for(struct ModuleList *l = *moduleList; l; l = l->next)
            if(pModule == l->pModule) {
                // Ya, we loaded this python module before.
                Py_DECREF(pModule);
                pModule = 0;
            }
        if(!pModule)
            // We have loaded this python module before so to keep this
            // loading as an independent module.  We load it from a copy
            // of the python file.
            LoadTmpCopy(modPath);
    }

    if(pModule == 0) {
        ERROR("Unable to load python module \"%s\"", moduleName);
        moduleList = 0;
        return -1; // error
    }

    // Add python controller module to the top of the list (stack).
    struct ModuleList *l = malloc(sizeof(*l));
    ASSERT(l, "malloc(%zu) failed", sizeof(*l));
    l->next = *moduleList;
    l->pModule = pModule;
    *moduleList = l;

    ERROR("Loaded python \"%s\": %p", moduleName, pModule);

    return 0;
}


void help(FILE *file) {

    // TODO: connect file to Python.
    WARN();

    PyObject *pFunc = PyObject_GetAttrString(pModule, "help");

    if(pFunc && PyCallable_Check(pFunc)) {
        PyObject *pValue = PyObject_CallObject(pFunc, 0);
        WARN();
        if(pValue)
            Py_DECREF(pValue);
    } else {
        if(pFunc)
            Py_DECREF(pFunc);
        ERROR("Cannot call python function help()");
    }

    if(pFunc)
        Py_DECREF(pFunc);
}



// https://stackoverflow.com/questions/21031856/python-embedding-passing-list-from-c-to-python-function
// Generic controller interface construct() function.
//
int construct(int argc, const char **argv) {

    PyObject *pFunc = PyObject_GetAttrString(pModule, "construct");

    int ret = 0; // default return value.
    
    if(pFunc && PyCallable_Check(pFunc)) {
    
        // args = [ arglist ] = [[argv[0], argv[1], ...]]
        PyObject *args = PyList_New(argc);
        ASSERT(args);
        for (Py_ssize_t i = 0; i < argc; ++i) {
            PyObject *item = PyUnicode_FromString(argv[i]);
            // arg steals the reference to item, so we do not Py_DECREF()
            // it.
            PyList_SetItem(args, i, item);
        }
        PyObject *arglist = Py_BuildValue("(O)", args);
        ASSERT(arglist);
        PyObject *result = PyObject_CallObject(pFunc, arglist);
        if(result) {
            ret = (int) PyLong_AsLong(result);
            NOTICE("Python %s() returned %d", __func__, ret);
            Py_DECREF(result);
        }
        Py_DECREF(args);
        Py_DECREF(arglist);
    }

    if(pFunc)
        Py_DECREF(pFunc);

    return ret;
}


int postStart(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInports, uint32_t numOutports) {


    return 1; // > 0  means do not call again until another start.
}


// Generic controller interface destroy() function.
//
int destroy(void) {

    if(pModule) {
        DASSERT(*moduleList);
        // Remove python module from the list (stack).
        struct ModuleList *prev = 0;
        struct ModuleList *l;
        for(l = *moduleList; l; l = l->next) {
            ERROR("l->next=%p  prev=%p", l->next, prev);
            if(pModule == l->pModule) {
                if(prev)
                    prev->next = l->next;
                else
                    *moduleList = l->next;
                free(l);
                break;
            }
            prev = l;
        }
        DASSERT(l);
        Py_DECREF(pModule);
        pModule = 0;
    }

    WARN();
    return 0;
}
