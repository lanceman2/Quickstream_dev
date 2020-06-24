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


void help(FILE *f) {
    fprintf(f,
"   Usage: pythonController\n"
"\n"
"   A controller module that wraps python code.\n"
"\n");
}


static PyObject *pModule;
static PyThreadState *threadState;
static PyInterpreterState *interp;

static void *
RunThread(void *data) {

    // create a new thread state for the the sub interpreter interp
    PyThreadState* ts = PyThreadState_New(interp);

    // make it the current thread state and acquire the GIL
    PyEval_RestoreThread(ts);

    // at this point:
    // 1. You have the GIL
    // 2. You have the right thread state 
    //
    // PYTHON WORK HERE
    PyObject *pFunc = PyObject_GetAttrString(pModule, "help");

    if(pFunc && PyCallable_Check(pFunc))
        PyObject_CallObject(pFunc, 0);

    PyObject_CallObject(pFunc, 0);

    Py_DECREF(pFunc);
    Py_DECREF(pModule);


    // clear ts
    PyThreadState_Clear(ts);

    // delete the current thread state and release the GIL
    PyThreadState_DeleteCurrent();

    return 0;
}


static
pthread_t thread;
static PyThreadState *main;


int pyInit(const char *moduleName) {

    WARN("moduleName=\"%s\"", moduleName);

    return 0;

    main = PyThreadState_Get();

    threadState = Py_NewInterpreter();
    ERROR("Py_NewInterpreter()=%p", threadState);


    PyObject *pName = PyUnicode_DecodeFSDefault(moduleName);
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    PyThreadState_Swap(main);

    if(pModule == 0) {
        ERROR("Unable to load python module \"%s\"", moduleName);
        return -1; // error
    }

    interp = threadState->interp;

    return 0;
}


// Generic controller interface construct() function.
//
int construct(int argc, const char **argv) {

    return 0;
}

int postStart(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInports, uint32_t numOutports) {

    return 1;

    ERROR();
    CHECK(pthread_create(&thread, 0, RunThread, 0));

    // This will unlock a mutex??
    PyEval_ReleaseThread(main);

    return 1; // > 0  means do not call again until another start.
}


// Generic controller interface destroy() function.
//
int destroy(void) {

    WARN();
    return 0;
}
