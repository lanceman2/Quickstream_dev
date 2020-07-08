/*
 * This file provides the C code that extends the "embedded Python"
 * controllers; that is quickstream controllers that are python files that
 * are loaded by quickstream and run as controllers.
 *
 * Specifically, this file provides the python callable functions that
 * python controller modules may call in python.  This is linked with the
 * pythonControllerLoader.so module and so each python controller module
 * that is loaded gets a shared instance of this code.  It's like the code
 * in this file is a singleton, you get one instance of this code.  If
 * distinct data is needed for a python module we must allocate it.
 *
 * PYTHONPATH or calling PySys_SetPath() is not needed to load this
 * module, because it is linked with the running program at run-time in
 * pythonControllerLoader.so.
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
#include "pyQsController.h"

// This file provides the scriptControllerLoader interface
// which we define in this header file:
#include "scriptControllerLoader.h"


void foo(void) {

}


// module state:
static int numargs=0;



/* Return the number of arguments of the application command line */
// implements method qsContoller::numargs()
//
static PyObject*
qsController_numargs(PyObject *self, PyObject *args)
{
    if(!PyArg_ParseTuple(args, ":numargs"))
        return NULL;
    return PyLong_FromLong(numargs++);
}

static PyMethodDef qsMethods[] = {
    {"numargs", qsController_numargs, METH_VARARGS,
     "Return the number of arguments received by the process."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef qsModule = {
    PyModuleDef_HEAD_INIT, "qsController", NULL, -1, qsMethods,
    NULL, NULL, NULL, NULL
};


// This is exposed to pythonControllerLoader.c
//
PyObject* qsPyControllerInitAPI(void)
{
    WARN("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& numargs=%d", numargs++);

    return PyModule_Create(&qsModule);
}
