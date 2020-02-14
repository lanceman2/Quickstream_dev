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
 */

/** filter module base class that is inherited by C++ filter
 * module classes
 *
 * \headerfile filter.hpp
 *
 * This is the base class that you can inherit to make a C++ filter
 * module.   You can use the CPP macro QS_LOAD_FILTER_MODULE()
 * to load your C++ object.
 */
class QsFilter {

    public:

        // We don't need a constructor.
        //QsFilter(void) { };

        /** filter's C++ input() function
         *
         * See the C \ref CFilterAPI::input() "input()" documentation for
         * details.
         * */
        virtual int input(void *buffers[], const size_t lens[],
            const bool isFlushing[], uint32_t numInPorts,
            uint32_t numOutPorts) = 0;

        /** filter's C++ help() function
         *
         * See the C \ref CFilterAPI::help() "help()" documentation for
         * details.
         */
        virtual void help(FILE *f) { };

        /** filter's C++ start() function
         *
         * See the C \ref CFilterAPI::start() "start()" documentation for
         * details.
         */
        virtual int start(uint32_t numInPorts, uint32_t numOutPorts) {
            return 0;
        };

        /** filter's C++ stop() function
         *
         * See the C \ref CFilterAPI::stop() "stop()" documentation for
         * details.
         *
         * */
        virtual int stop(uint32_t numInPorts, uint32_t numOutPorts) {
            return 0;
        };

        /** filter's C++ base ~QsFilter() destructor function
         *
         */
        virtual ~QsFilter(void) { };
};



/** C++ loader CPP (C preprocessor) macro to create your C++ filter object
 *
 * The C++ loader CPP (C preprocessor) macro that will create a C++
 * superclass object from the quickstream module loader function
 * qsAppLoadFilter() of the from the \ref quickstream_program.
 */
#define QS_LOAD_FILTER_MODULE(ClassName) \
    extern "C" {\
    int construct(int argc, const char **argv) {\
        f = new ClassName(argc, argv);\
        return 0;\
    }\
}


#ifndef DOXYGEN_SHOULD_SKIP_THIS

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

#endif // #ifndef DOXYGEN_SHOULD_SKIP_THIS


#endif // #ifndef __qsfilter_hpp__
