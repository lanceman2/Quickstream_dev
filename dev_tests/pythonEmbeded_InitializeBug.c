// cc -Wall -g -Wall $(python3-config --includes) pythonEmbeded_InitializeBug.c -o pythonEmbeded_InitializeBug $(python3-config --embed --libs)

#include <stdio.h>

#include <Python.h>


int main(void) {

    //Py_Initialize();
    Py_InitializeEx(0);

    fprintf(stderr, "calling Py_FinalizeEx()\n");
    Py_FinalizeEx();
    fprintf(stderr, "called Py_FinalizeEx()\n");

    return 0;
}
