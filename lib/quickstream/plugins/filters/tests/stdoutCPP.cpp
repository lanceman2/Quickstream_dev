#include <iostream>
#include <bitset>

#include "quickstream/filter.h"
#include "quickstream/filter.hpp"


class Stdout: public QsFilter {

    public:

    Stdout(int argc, const char **argv) {
        // Parse args if you like ...
    };


    int start(uint32_t numInPorts, uint32_t numOutPorts) {

        if(numInPorts < 1) {
            std::cerr << "No inputs" << std::endl;
            return -1; // error fail
        }
        if(numOutPorts != 0) {
            std::cerr << "There should be no outputs "
                << numOutPorts  << " found" << std::endl;
            return -1; // error fail
        }
        return 0; // success
    }


    int input(void *inBuffers[], const size_t inLens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

        size_t len = (size_t) inLens[0];
        if(len > QS_DEFAULTMAXWRITE)
            len = QS_DEFAULTMAXWRITE;

        std::cout.write((const char*) inBuffers[0], len);

        qsAdvanceInput(0, len);

        if(std::cout.eof()) {
            return 1; // done
        }

        return 0; // success continue
    };


    void help(FILE *file) {

        fprintf(file,
"  Usage: stdoutCPP\n"
"\n"
"  Reads 1 input and writes that to stdout.  This filter can have no\n"
"  outputs.\n"
"\n"
        );
    };
};


// QS_LOAD_FILTER_MODULE will add the needed code to make this a loadable
// quickstream module.
//
// We do not want a semicolon after this CPP macro
QS_LOAD_FILTER_MODULE(Stdout)

