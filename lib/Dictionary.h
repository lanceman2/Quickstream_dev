
struct QsDictionary;


// This Dictionary class is filled with key/value pairs and then built.
// After it is built no more key/value pairs are Inserted.

extern
struct QsDictionary *qsDictionaryCreate(void);

extern
void qsDictionaryDestroy(struct QsDictionary *dict);

// Insert key/value if not present.
// 
// Returns 0 on success or 1 if already present and -1 if it is not added
// and have an invalid character.
extern
int qsDictionaryInsert(struct QsDictionary *dict,
        const char *key, const void *value);

// This is the fast Find() function.
//
// Returns element value for key or 0 if not found.
extern
void *qsDictionaryFind(const struct QsDictionary *dict, const char *key);

extern
void qsDictionaryPrintDot(const struct QsDictionary *dict, FILE *file);


// Searches through the whole data structure.
//
// When key is non-zero, each call to callback() it calls with realloc()
// allocated memory, and the memory is reused.  After the last call to
// callback() the user must free() the last returned key memory.  If the
// user needs more than one key, then the user must copy the key to their
// own memory.
//
// If callback() returns non-zero than the search will stop and callback
// will not be called again.  The user owns the last returned key and must
// free() it some time.
//
// Searches the entire data structure starting at dict.  Calls callback
// with key set (if non-zero) and value 
//
extern 
void
qsDictionaryForEach(const struct QsDictionary *dict,
        int (*callback) (const char **key, void *value));
