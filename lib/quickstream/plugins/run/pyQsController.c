/*
 * This file provides the C code that extends the "embedded Python"
 * controllers; that is quickstream controllers that are python files that
 * are loaded by quickstream and run as controllers.  This so loaded
 * python scripts may "import pyQsController" to get the python that
 * this file adds.
 *
 * Example:

 import pyQsController

 controller = pyQsController.create()


 controller.getParameter(...)


 *
 *
 *************************************************************************
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
//#include <stdatomic.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

// The public installed user interfaces:
#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/parameter.h"


// Private interfaces.
#include "../../../debug.h"
#include "../../../Dictionary.h"
#include "../../../qs.h"
#include "../../../LoadDSOFromTmpFile.h"
#include "../../../controller.h"

// This file provides the scriptControllerLoader interface
// which we define in this header file:
#include "scriptControllerLoader.h"

#include "pythonController.h"


// simple example:
// Coding the Methods of a Python Class in C
// https://www.oreilly.com/library/view/python-cookbook/0596001673/ch16s06.html
//
// Opaque Pointers in C extension modules
// https://www.geeksforgeeks.org/python-opaque-pointers-in-c-extension-modules/
// 
// reference:
// https://docs.python.org/3/extending/extending.html#a-simple-example
//
// What is the PyClass_New equivalent in Python 3?
// Or how to make a class object with python C API:
//
// https://stackoverflow.com/questions/29301824/what-is-the-pyclass-new-equivalent-in-python-3#30626414

// :::IMPORTANT NOTE:::
// Python Decorators "@"
// https://www.programiz.com/python-programming/decorator


/*
 * The python code that this file creates is not added to python scripts
 * without the script calling "import pyQsController".
 *
 * Python 3.5 can have module that load new objects for each import:
 *
 * PEP 489 -- Multi-phase extension module initialization:
 * https://www.python.org/dev/peps/pep-0489/
 * Failed to get that to work.
 *
 * PyThreadState* Py_NewInterpreter()
 *
 *
 * Python | Opaque Pointers in C extension modules
 * https://www.geeksforgeeks.org/python-opaque-pointers-in-c-extension-modules/
 *
 *
 * we'll need to use Py_NewInterpreter()
 *
 * https://docs.python.org/3/c-api/init.html
 */
// We do not want just a module.  Because modules only have one instance
// for a given interpreter.  We want to be able to load any number of
// python scripts which have different instances of class objects in them.
// This module is shared between the python scripts that we load:
// https://www.python.org/dev/peps/pep-0489/
// with PEP 489 -- Multi-phase extension module initialization, we can
// have modules that can load an instance at each time the module is
// imported.  But we we tried and have not seen a working example of this.


static inline void PrintObj(PyObject *obj) {
    PyObject* repr = PyObject_Repr(obj);
    PyObject* str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
    const char *bytes = PyBytes_AS_STRING(str);

    WARN("REPR: %s", bytes);

    Py_XDECREF(repr);
    Py_XDECREF(str);
}


#if 1
// Python module method.
static PyObject *getVersion(PyObject *self, PyObject *args) {

    PrintObj(args);
    PrintObj(self);

    WARN();
    return Py_BuildValue("s", QS_VERSION);
}
#endif


// This code started with:
// https://docs.python.org/3.8/extending/newtypes_tutorial.html


struct Controller {
    PyObject_HEAD
    struct QsController *controller;
    int number;
};

static void
Controller_dealloc(struct Controller *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Controller_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    struct Controller *self;
    self = (struct Controller *) type->tp_alloc(type, 0);
    return (PyObject *) self;
}

static int
Controller_init(struct Controller *self, PyObject *args, PyObject *kwds) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");

    self->controller = currentController;
    return 0;
}

static PyMemberDef Controller_members[] = {
    {"number", T_INT, offsetof(struct Controller, number), 0,
     "custom number"},
    {NULL}  /* Sentinel */
};

static PyGetSetDef Controller_getsetters[] = {
    {NULL}  /* Sentinel */
};


struct GetCallback {

    PyObject *pyFunction;
    // A python tuple of arguments that is passed to the callback
    // on each call.
    PyObject *userArgs;

};


static void getCallback_cleanup(struct GetCallback *userData) {

    ERROR();

    DASSERT(userData);
    if(userData->userArgs) {
        Py_DECREF(userData->userArgs);
        userData->userArgs = 0;
    }
    if(userData->pyFunction) {
        Py_DECREF(userData->pyFunction);
        userData->pyFunction = 0;
    }

    free(userData);
}


static int getCallback(
            const void *value,
            void *streamOrApp,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData) {

    struct GetCallback *cb = userData;
    PyObject *val;

    switch(type) {

        case QsDouble:
            // Pack a double into the python args
            val = PyFloat_FromDouble(*((double *) value));
            break;

        case QsUint64:
            // Pack a uint64_t into the python args
            val = PyLong_FromUnsignedLong(*((uint64_t *) value));
            break;

        default:
            ASSERT(0, "Unknown type. Write more code here.");
    }

    DASSERT(val);

    // The first argument to the python function is this
    // value we just got.
    ASSERT(PyTuple_SetItem(cb->userArgs, 0, val) == 0);

    ERROR("\n\n-----------------------------------------------------------------------------------------------------------------------------------------------------------");

    PyObject *result = PyObject_CallObject(cb->pyFunction, cb->userArgs);

PrintObj(0);
PrintObj(result);
PrintObj(cb->pyFunction);
PrintObj(cb->userArgs);


    if(result) {
        int ret = (int) PyLong_AsLong(result);
        Py_DECREF(result);

    ERROR("ret=%d", ret);

        return ret;
    }

    ERROR("\n-----------------------------------------------------------------------------------------------------------------------------------------------------------");


    return 0;
}


static PyObject *
Controller_getParameter(struct Controller *self, PyObject *args) {

    PrintObj((PyObject *) self);
    PrintObj(args);

    Py_ssize_t len = PyTuple_Size(args);
    ASSERT(len >= 4);

    PyObject *userArgs = PyTuple_New(len);
    ASSERT(userArgs);

    PyObject *o = PyTuple_GetItem(args, 0);
    ASSERT(o);
    // Add streamOrApp to user args in 2nd (1) position.
    //
    // In order for PyTuple_SetItem() to "steal" this object we need to
    // own a reference to it now by calling Py_INCREF(o).
    Py_INCREF(o);
    ASSERT(PyTuple_SetItem(userArgs, 1, o) == 0);

    void *streamOrApp = PyCapsule_GetPointer(o, 0);
    ASSERT(streamOrApp);
    const char *filterName; // or controller name
    o = PyTuple_GetItem(args, 1);
    ASSERT(o);

    if(((struct QsStream *)streamOrApp)->type == _QS_STREAM_TYPE) {

        // Filters are managed by streams.
        struct QsFilter *f = PyCapsule_GetPointer(o, 0);
        ASSERT(f);
        filterName = f->name;

     } else if(((struct QsStream *)streamOrApp)->type ==
             _QS_APP_TYPE) {

        // Controllers are managed by apps.
        struct QsController *c = PyCapsule_GetPointer(o, 0);
        ASSERT(c);
        filterName = c->name;
     } else {
         ASSERT(0, "object not stream or app");
     }

    // Add filterName to user args in 3rd (2) position.
    PyObject *str = PyUnicode_DecodeUTF8(filterName,
            strlen(filterName), 0);
    DASSERT(str);
    // We now own a reference to str, and will pass that responsibility
    // to the userArgs object.
    //
    // userArgs receives ownership of the reference to str, so we assume
    // that calling Py_DECREF(userArgs) with effectively call
    // Py_DECREF(str).
    ASSERT(PyTuple_SetItem(userArgs, 2, str) == 0);

    // Get the parameter name argument.
    o = PyTuple_GetItem(args, 2);
    ASSERT(o);
    PrintObj(o);
    const char *pName = PyUnicode_AsUTF8(o);
    // TODO: figure out who manages the memory to pName.  Is is just a
    // pointer to magic memory?
    ASSERT(pName);
    // Add parameter Name to user args in 4th (3) position.
    // We need to own if first:
    Py_INCREF(o);
    ASSERT(PyTuple_SetItem(userArgs, 3, o) == 0);

    o = PyTuple_GetItem(args, 3);
    ASSERT(o);

    struct GetCallback *userData = malloc(sizeof(*userData));
    ASSERT(userData, "malloc(%zu) failed", sizeof(*userData));
    // the rest of the python arguments that will be passed back to the
    // getCallback function.

    userData->pyFunction = PyTuple_GetItem(args, 3);
    ASSERT(userData->pyFunction);
    Py_INCREF(userData->pyFunction);
    PrintObj(userData->pyFunction);
    userData->userArgs = userArgs;

    if(len > 4) {
        // We add the rest of the items in args to the arguments that will
        // be passed to the python users callback.  This will let the user
        // pass anything to their callbacks.
        //
        for(Py_ssize_t i=4; i<len; ++i) {
            o = PyTuple_GetItem(args, i);
            DASSERT(o);
            Py_INCREF(o); // own it
            // pass ownership to the userArgs.
            ASSERT(PyTuple_SetItem(userArgs, i, o) == 0);
            PrintObj(userArgs);
        }
    }

    qsParameterGet(streamOrApp, filterName, pName, Any, getCallback,
            (void (*)(void *)) getCallback_cleanup, userData, 0);
 ERROR(" ===========================================================================================================");

    PrintObj(userArgs);

    Py_INCREF(Py_None);
    return Py_None;
}



static PyMethodDef Controller_methods[] = {
    { "getParameter", (PyCFunction) Controller_getParameter,
        METH_VARARGS, "getParameter"
    },
    {NULL}  /* Sentinel */
};


static PyTypeObject ControllerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pyQsController.Controller",
    .tp_doc = "Controller objects",
    .tp_basicsize = sizeof(struct Controller),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Controller_new,
    .tp_init = (initproc) Controller_init,
    .tp_dealloc = (destructor) Controller_dealloc,
    .tp_members = Controller_members,
    .tp_methods = Controller_methods,
    .tp_getset = Controller_getsetters,
};


static PyMethodDef moduleMethods[] = {
    {"getVersion",  getVersion, METH_VARARGS,
     "Get the quickstream module version as a string."},
    { 0, 0, 0, 0 } /* null terminate */
};

static PyModuleDef moduleDef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pyQsController",
    .m_doc = "Example module that creates an extension type.",
    .m_size = -1,
    .m_methods = moduleMethods
};


PyMODINIT_FUNC
PyInit_pyQsController(void) {

    PyObject *m;
    if (PyType_Ready(&ControllerType) < 0)
        return NULL;

    m = PyModule_Create(&moduleDef);
    if (m == NULL)
        return NULL;

    Py_INCREF(&ControllerType);
    if (PyModule_AddObject(m, "Controller", (PyObject *) &ControllerType) < 0) {
        Py_DECREF(&ControllerType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
