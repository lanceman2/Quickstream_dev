#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../include/quickstream/filter.h"
#include "debug.h"
#include "qs.h"
#include "filterAPI.h" // struct QsJob *GetJob(void){}




int qsParameterCreate(struct QsStream *s, const char * Class,
        const char *name, void *value) {

    if(s == 0) {
        // This may be a filter request.
        s = GetStream();
        DASSERT(s);
    }


    return 0; // success
}


void *qsParameterGet(struct QsStream *s, const char * Class,
        const char *name,
        int (*callback)(void *retValue)) {


    return 0;
}


int qsParameterSet(struct QsStream *s, const char * Class,
        const char *name, void *value,
        int (*callback)(void *retValue)) {


    return 0; // success
}
