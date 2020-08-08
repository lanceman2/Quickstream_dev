// See help() function for details.

#include <iostream>
#include <bitset>
#include <inttypes.h>
#include <uhd/usrp/multi_usrp.hpp>

#include "../../../../../include/quickstream/filter.h"
#include "../../../../../include/quickstream/filter.hpp"
#include "../../../../../include/quickstream/parameter.h"
#include "../../../../../lib/debug.h"

// This is a little weird writing C++ with a C API.  The fact that we are
// making most of the data in the Rx class public does not matter much
// given that this code is loaded in it's own "running namespace" by the
// linker loader API, and so none of the data and functions are exposed to
// the rest of the running program except the Rx methods start(), stop(),
// input(), and help().  Except for those functions (methods) the dlopen()
// linker loader API hides all the data and functions in this file
// unexposed to the rest of the running program.

extern "C" {

static int setFreq(struct QsParameter *parameter,
            void *value, const char *pName,
            void *rxObject) {

    return 0; // success and continue
}

static int setRate(struct QsParameter *parameter,
            void *value, const char *pName,
            void *rxObject) {

    return 0; // success and continue
}

static int setGain(struct QsParameter *parameter,
            void *value, const char *pName,
            void *rxObject) {

    return 0; // success and continue
}


}; // close extern "C" {}



class Rx: public QsFilter {

    public:

    double freq, rate, gain;

    Rx(int argc, const char **argv) {

        freq = qsOptsGetDouble(argc, argv, "freq", 914.5e+6);
        rate = qsOptsGetDouble(argc, argv, "rate", 2.0e+6);
        gain = qsOptsGetDouble(argc, argv, "gain", 0.0);

        qsParameterCreate("freq", QsDouble, setFreq, 0, this);
        qsParameterCreate("rate", QsDouble, setRate, 0, this);
        qsParameterCreate("gain", QsDouble, setGain, 0, this);
    };


    int start(uint32_t numInPorts, uint32_t numOutPorts) {

        if(numInPorts > 1) {
            std::cerr << "No inputs" << std::endl;
            return -1; // error fail
        }
        if(numOutPorts != 1) {
            std::cerr << "There should be 1 output "
                << numOutPorts  << " found" << std::endl;
            return -1; // error fail
        }
        return 0; // success
    }


    int input(void *inBuffers[], const size_t inLens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {


        return 0; // success continue
    };


    void help(FILE *file) {

        fprintf(file,
"  Usage: rx { --freq FREQ_HZ --rate RATE_HZ --gain GAIN_DB }\n"
"\n"
"  This is a source (no inputs) quickstream filter module and has as many output as\n"
"  it is configured by command-line arguments and what-ever.  It uses the libUHD library\n"
"  (which is not a kernel driver as the documents imply by calling it the \"USRP Hardware\n"
"  Driver\") to access Ettus Reasearch hardware so called USRPs (Universal Software Radio\n"
"  Peripheral) which enable the development of SDRs (software defined radios).  As best I\n"
"  can tell the interface from this libuhd to the USRPs is just UDP (User Datagram\n"
"  Protocol) over Ethernet.  The documents tend to mystify such details.\n"
"\n"
"  References: https://www.ettus.com/  https://files.ettus.com/manual/index.html\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"       TODO: Write this OPTIONS code.\n"
"\n"
"       TODO: Add uhd channels; not just 1 channel.\n"
"\n"
"\n"
     
        );
    };
};


// QS_LOAD_FILTER_MODULE will add the needed code to make this a loadable
// quickstream module.
//
// We do not want a semicolon after this CPP macro
QS_LOAD_FILTER_MODULE(Rx)
