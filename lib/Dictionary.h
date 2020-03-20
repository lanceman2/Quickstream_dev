
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
// Do not screw with the key memory that is passed to callback().
//
// If callback returns non-zero the callback stops being called and the
// call to qsDictionaryForEach() returns.
//
// Searches the entire data structure starting at dict.  Calls callback
// with key set (if non-zero) and value.
//
// Returns the number of keys and callbacks called.
//
extern 
size_t
qsDictionaryForEach(const struct QsDictionary *dict,
        int (*callback) (const char *key, const void *value));
