/* Header file to include in modules that load controllers that are
 * scripts.
 *
 * Including this will keep the module interfaces consistent at compile
 * time. */

int initialize(void);

void *loadControllerScript(const char *path, int argc, const char **argv);

void cleanup(void);
