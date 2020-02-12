#ifndef __qsfilter_hpp__
#define __qsfilter_hpp__

#ifndef __cplusplus
#  error "This is C++ not C"
#endif


#include <stdio.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>


/** \file
 *
 * \brief filter C++ module writers application programming interface (API)
 */

/** quickstream filter module base class that is inherited by C++ filter
 * module classes
 *
 */
class QsFilter {

    public:

        QsFilter(void) { };

        virtual int input(void *buffers[], const size_t lens[],
            const bool isFlushing[], uint32_t numInPorts,
            uint32_t numOutPorts) = 0;

        virtual void help(FILE *f) { };

        virtual int start(uint32_t numInPorts, uint32_t numOutPorts) {
            return 0;
        };

        virtual int stop(uint32_t numInPorts, uint32_t numOutPorts) {
            return 0;
        };

        virtual ~QsFilter(void) { };
};


#define QS_LOAD_FILTER_MODULE(xxx) \
    extern "C" {\
    int construct(int argc, const char **argv) {\
        f = new xxx(argc, argv);\
        return 0;\
    }\
}


extern "C" {

static class QsFilter *f;


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    return f->input(buffers, lens, isFlushing, numInPorts, numOutPorts);
}

int start(uint32_t numInPorts, uint32_t numOutPorts) {

    return f->start(numInPorts, numOutPorts);
}

int stop(uint32_t numInPorts, uint32_t numOutPorts) {

    return f->stop(numInPorts, numOutPorts);
}

void help(FILE *file) {

    f->help(file);
}

int destroy(void) {

    DASSERT(f);
    delete f;
    f = 0;
}


} // extern "C"

#endif // #ifndef __qsfilter_hpp__
