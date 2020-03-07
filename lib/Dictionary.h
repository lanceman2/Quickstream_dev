
struct QsDictionary;


// This Dictionary class is filled with key/value pairs and then built.
// After it is built no more key/value pairs are Inserted.

extern
struct QsDictionary *qsDictionaryCreate(void);

extern
void qsDictionaryDestroy(struct QsDictionary *dict);

// Insert key/value if not present.
// Returns 0 if this key is present already.
extern
void *qsDictionaryInsert(struct QsDictionary *dict,
        const char *key, const void *value);

// Returns element ptr.
extern
void *qsDictionaryFind(struct QsDictionary *dict, const char *key);
