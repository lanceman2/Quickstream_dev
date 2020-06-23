/* this defines the pythonController interfaces; since the
 * there is a script loader module that calls functions from the
 * python script wrapper module. */

// The script wrapper must have this.  This could have python objects
// added to its' arguments, hence we call it pyInit.
int pyInit(const char *pyPath);
