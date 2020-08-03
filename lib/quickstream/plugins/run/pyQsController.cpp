/*
 * This file provides the C code that extends the "embedded Python"
 * controllers; that is quickstream controllers that are python files that
 * are loaded by quickstream and run as controllers.  This so loaded
 * python scripts may "import pyQsController" to get the python that
 * this file adds.
 *
 * The python code that this file creates is not added to python scripts
 * without the script calling "import pyQsController".
 *
 * Specifically, this file provides the python callable functions that
 * python controller modules may call in python.
 *
 * The controller DSO pythonController.so adds the directory that this
 * file is in to the PYTHON PATH (python module searching path).
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

#define PY_SSIZE_T_CLEAN
#include <Python.h>

// The public installed user interfaces:
#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/parameter.h"


extern "C" {

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

} // extern "C"

// ref: best simple example:
//
// https://blog.ekbana.com/write-a-python-binding-for-your-c-code-using-pybind11-library-ef0992d4b68


#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

namespace py = pybind11;


//static int count = 0;

// Python module wide method.
const char *getVersion(void){

    return QS_VERSION;
}


static int CapsuleCount = 0;



struct GetCallbackData {

    PyObject *userData;

    std::function<int(uint64_t, void *, const char *,
                const char *,  void *)> getPyCallback;

};


int getCallback(const void *value, void *streamOrApp,
        const char *ownerName,
        const char *pName, 
        enum QsParameterType type,
        struct GetCallbackData *getCallbackData) {

    DASSERT(getCallbackData);

    char str[16];
    snprintf(str, 16, "%" PRIu32, CapsuleCount++);
    PyObject *s = PyCapsule_New(streamOrApp, str, 0);
ERROR();
    return getCallbackData->getPyCallback(*((uint64_t *) value),
        (void *) s, ownerName, pName, (void *) getCallbackData->userData
        );
}


static void getParameterCleanup(struct GetCallbackData *getCallbackData) {
    DASSERT(getCallbackData);
ERROR("GetCallbackData=%p  sizeof(struct GetCallbackData)=%zu",
        getCallbackData, sizeof(struct GetCallbackData));
    delete getCallbackData;
ERROR();
}


class Controller {

    public:
    
        Controller(struct QsController *c);
        ~Controller(void);

        int getParameter(void *filter, const char *pName,
                std::function<int(uint64_t, void *, const char *,
                    const char *,  void *)>
                getPyCallback, PyObject *userData);

    private:

        struct QsController *controller;
};


Controller::Controller(struct QsController *c) {

    //WARN("**************c=%p pyQsCount=%d", c, count++);

    controller = c;
}

Controller::~Controller(void) {

    ERROR();
}



int Controller::getParameter(void *filter, const char *pName,
        std::function<int(uint64_t, void *, const char *,
        const char *,  void *)> getPyCallback, PyObject *userData) {

    struct QsFilter *f = (struct QsFilter *) filter;

    ERROR("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ parameter=%s:%s", f->name, pName);


    struct GetCallbackData *getCallbackData =
            // We tried malloc() but there's something funny about
            // what pybind11 does with that, and the program
            // compiled but crashed.
            new struct GetCallbackData;
ERROR("GetCallbackData=%p", getCallbackData);
            getCallbackData->getPyCallback = getPyCallback;
            getCallbackData->userData = (PyObject *) userData;
ERROR();
    DASSERT(f);
    DASSERT(f->stream);
    DASSERT(f->name);

    ERROR();

    return qsParameterGet(f->stream, f->name, pName, QsUint64,
            (int (*)(
                const void *value, void *streamOrApp,
                const char *ownerName, const char *pName, 
                enum QsParameterType type, void *userData)
            ) getCallback,
            (void (*)(void *)) getParameterCleanup,
            (void *) getCallbackData,
            QS_KEEP_ONE);
}


/*
 * Generate the python bindings for this python module.
 *
 * This file is loaded by the python interpretor only once, even if it is
 * imported by more than one python script.  So if we need different state
 * data for each python script we must allocate data in this file.
 *
 * pyQsController is the name of the python module, and must be the name of
 * this file too, as in:
 *
 *     import pyQsController
 *
 *
 * m is the local module object thingy.
 *
 */
PYBIND11_MODULE(pyQsController, m) {
    //m.doc() = "Get quickstream version as a string";


    // currentController must be set in other C files before the python
    // script makes a this python version of a Controller
    py::class_<Controller>(m, "Controller")
        .def(py::init([]() {
            DASSERT(currentController);
            return new Controller(currentController);
        }))
        .def("getParameter", &Controller::getParameter, "help");

    m.def("getVersion", &getVersion,
            "Get quickstream version as a string");
}
