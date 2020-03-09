
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

// Returns element value for key.
extern
void *qsDictionaryFind(const struct QsDictionary *dict, const char *key);

extern
void qsDictionaryPrintDot(const struct QsDictionary *dict, FILE *file);
