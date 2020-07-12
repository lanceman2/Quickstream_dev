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
