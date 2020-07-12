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

} // extern "C"


#include <pybind11/pybind11.h>


namespace py = pybind11;


const char *getVersion(void){

    DSPEW();

    return QS_VERSION;
}

/**
 * Generate the python bindings for this C++ function
 */
PYBIND11_MODULE(pyQsController, m) {
    m.doc() = "Get quickstream version as a string";

    m.def("getVersion", &getVersion,
            "Get quickstream version as a string");
}
