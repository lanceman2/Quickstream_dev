#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <alloca.h>

#include "./qs.h"
#include "../include/qsfilter.h"
#include "./debug.h"

#if 0
static inline void
AdvanceWritePtr(struct QsOutput *output, size_t len) {

    // We are in the correct thread to do this.
    struct QsWriter *writer = output->writer;
    struct QsBuffer *buffer = writer->buffer;

    DASSERT(len <= buffer->overhangLength,
            "len=%zu > buffer->overhangLength=%zu",
            len, buffer->overhangLength);

    if(writer->writePtr + len < buffer->mem + buffer->mapLength)
        writer->writePtr += len;
    else
        writer->writePtr += len - buffer->mapLength;
}


static inline void
AdvanceReadPtr(struct QsOutput *output, size_t len) {

    // We are in the correct thread to do this.
    struct QsBuffer *buffer = output->writer->buffer;

    DASSERT(len <= buffer->overhangLength,
            "len=%zu > buffer->overhangLength=%zu",
            len, buffer->overhangLength);

    if(output->readPtr + len < buffer->mem + buffer->mapLength)
        output->readPtr += len;
    else
        output->readPtr += len - buffer->mapLength;
}
#endif
