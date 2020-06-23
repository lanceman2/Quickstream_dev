// quickstream controller module that add a byte counter for every filter
// input and output port.
//
// This module should be able to work with more than one stream running.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Python.h>


#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../debug.h"

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


PyObject *pModule;


int pyInit(const char *moduleName) {

    WARN("moduleName=\"%s\"", moduleName);

    PyObject *pName = PyUnicode_DecodeFSDefault(moduleName);
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if(pModule == 0) {
        ERROR("Unable to load python module \"%s\"", moduleName);
        return -1; // error
    }

    return 0;
}


// Generic controller construct() function that is not python specific.
//
int construct(int argc, const char **argv) {

    return 0;
}


int destroy(void) {

    Py_DECREF(pModule);
    WARN();
    return 0;
}
