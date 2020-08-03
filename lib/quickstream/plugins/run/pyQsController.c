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


#if 0
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


static PyObject *
Controller_getParameter(struct Controller *self, PyObject *args) {

    PrintObj((PyObject *) self);
    PrintObj(args);

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

static PyModuleDef moduleDef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pyQsController",
    .m_doc = "Example module that creates an extension type.",
    .m_size = -1,
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
