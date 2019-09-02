#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

#include "./qsfilter.h"
#include "./debug.h"


__thread struct QsFilter *_qsCurrentFilter = 0;


void *qsBufferGet(size_t *len, uint32_t outputChannelNum) {

    DASSERT(_qsCurrentFilter,"");

    return 0;
}


void qsOutput(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsCurrentFilter,"");


}




