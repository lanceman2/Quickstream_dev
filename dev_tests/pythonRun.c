// reference:
// https://docs.python.org/3/extending/embedding.html

// Run 'make' and
// ./pythonRun multiply  multiply 3 4

#include <stdlib.h>

//#define PY_SSIZE_T_CLEAN
#include <Python.h>

int
main(int argc, char *argv[])
{
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    if (argc < 3) {
        fprintf(stderr,"Usage: %s pythonfile funcname [args]\n",
                argv[0]);
        return 1;
    }

    // PYTHON 3.8.2 BUG: So long as PYTHONPATH is not set we cannot load
    // ./multiply.py
#if 1
    setenv("PYTHONPATH", ".:..:", 1);
#endif
    // skips initialization registration of signal handlers.
    Py_InitializeEx(0);
#if 0
    setenv("PYTHONPATH", ".", 1);
#endif
 
    Py_Initialize();

    pName = PyUnicode_DecodeFSDefault(argv[1]);
    /* Error checking of pName left out */
    fprintf(stderr, "pName=%p\n", pName);

    pModule = PyImport_Import(pName);

    fprintf(stderr, "pModule=%p\n", pModule);

    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, argv[2]);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(argc - 3);
            for (i = 0; i < argc - 3; ++i) {
                pValue = PyLong_FromLong(atoi(argv[i + 3]));
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
                    fprintf(stderr, "Cannot convert argument\n");
                    return 1;
                }
                /* pValue reference stolen here: */
                PyTuple_SetItem(pArgs, i, pValue);
            }
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
                return 1;
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", argv[2]);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", argv[1]);
        return 1;
    }
    if (Py_FinalizeEx() < 0) {
        return 120;
    }
    return 0;
}
