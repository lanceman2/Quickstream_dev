// quickstream controller module that add a byte counter for every filter
// input and output port.
//
// This module should be able to work with more than one stream running.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../debug.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: pythonController\n"
"\n"
"   A controller module that wraps python code.\n"
"\n");
}

