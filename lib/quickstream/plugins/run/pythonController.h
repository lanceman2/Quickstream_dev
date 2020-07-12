/* this defines the pythonController interfaces; since the
 * there is a script loader module that calls functions from the
 * python script wrapper module. */

// To keep a list of Python controller modules loaded.
//
// The reason we needed a list was to check if a python module that is
// loaded has been loaded before, and if so we must reload it from a
// temporary copy of the python file.  Otherwise we end up with python
// code (modules) not being able to be loaded more than once.
//
struct ModuleList {

    PyObject *pModule;
    struct ModuleList *next;
};

// The script wrapper must have this.  This could have python objects
// added to its' arguments, hence we call it pyInit.
int pyInit(const char *pyPath, struct ModuleList **moduleList,
        const char *controllerDSODir);
