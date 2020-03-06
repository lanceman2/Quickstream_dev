
struct QsDictionary;


extern
struct QsDictionary *qsDictionaryCreate(void);

extern
void qsDictionaryDestroy(struct QsDictionary *dict);

// Returns ptr.
extern
void *qsDictionaryInsert(struct QsDictionary *dict,
        const char *key, const void *ptr);

// Returns element ptr.
extern
void *qsDictionaryFind(struct QsDictionary *dict, const char *key);
