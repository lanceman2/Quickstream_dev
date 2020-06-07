// Reference:
// http://fftw.org/fftw3_doc/Complex-DFTs.html
/*
{
    fftw_complex *in, *out;
    fftw_plan p;
    ...
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    ...
    fftw_execute(p); // repeat as needed
    ...
    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);
}
*/

// The "regular" fftw API interfaces does not let you pass in and out new
// arrays without remaking a plan (reconfiguring).  I do not think it's
// thread safe either.
//
// Okay fftw_plan fftw_plan_many_dft() and fftw_execute_dft() are thread
// safe.  They are part of the "Advanced Interface".  Looks like I need
// fftw_alignment_of() too, at least to test this.

#include <complex.h>
#include <fftw3.h>


#include "../../../../include/quickstream/filter.h"
#include "../../../../lib/debug.h"



static int bins = 512;
static size_t maxWrite;
static fftwf_plan plan;
static size_t bufMult = 2;
static size_t batchSize;


void help(FILE *f) {

    fprintf(f,

"    Usage: fftwComplexFloat1D { --maxWrite LEN }\n"
"\n"
"  This has one input and one output.\n"
"\n"
"  It assumes that the input is a series 2 floats (I/Q).\n"
"  It output a series of arrays of size NUM I/Q floats.\n"
"\n"
"  Add more output configuration options and parameters.\n"
"\n"
"  TODO: write this with a pass-through buffer.  The\n"
"  input data and output data are the same size.\n"
"\n"
"  There's a lot more that we could add to this, but we need\n"
"  to be careful not to add things that could better be done\n"
"  in a down-stream or up-stream filter.  For example:\n"
"  averaging, calculating power, and interlacing the input.\n"
"\n"
"\n"
"                    OPTIONS\n"
"\n"
"  --bins NUM     NUM is the number of I/Q pairs used to compute\n"
"                 each FFT.  The default NUM is %d.\n"
"\n"
"\n"
"  --bufMult N    N is the maximum number of FFT calculations that\n"
"                 can be written in on input call.  The default is\n"
"                 %zu.  From this, N, and NUM the maximum output\n"
"                 buffer size is set to N*NUM.\n"
"\n"
"\n",
    bins, bufMult);

}




int construct(int argc, const char **argv) {

    bins = qsOptsGetInt(argc, argv, "bins", bins);
    bufMult = qsOptsGetSizeT(argc, argv, "bufMult", bufMult);

    // Ya, whatever.
    ASSERT(bins >= 2);
    ASSERT(bins < 10*1024);
    ASSERT(bufMult >= 1);
    ASSERT(bufMult < 1000);

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 1);
    ASSERT(numOutPorts == 1);

    // Stupid fftw interface:
    //
    // I think we can use any aligned float complex pointers where in !=
    // out.  They (in and out) can't have the same address.  I think the
    // only reason that it wants to look at the in and out array in
    // fftw_plan_many_dft() is to test if that are using the same array,
    // whereby it would setup so that we have output overwrite the input
    // array.   We may want that if we can figure out how to use a pass
    // through stream buffer.
    //
    float complex in[2];
    float complex out[2];

    // We define batchSize as the length, in bytes, of an input and output
    // array for single batch of complex FFT.
    //
    batchSize = bins * sizeof(float complex);

    /* fftw_plan_many_dft( int rank, const int *n, int howmany,
                             fftw_complex *in, const int *inembed,
                             int istride, int idist,
                             fftw_complex *out, const int *onembed,
                             int ostride, int odist,
                             int sign, unsigned flags)

    int fftw_alignment_of(double *p) ??

    */
    plan = fftwf_plan_many_dft(1, &bins, 1,
            in, 0, 1, 0,
            out, 0, 1, 0,
            -1 /*sign*/, FFTW_ESTIMATE /*flags*/);


    // Make maxWrite a multiple of bins*sizeof(float complex)

    maxWrite = bufMult * batchSize;

    // We need at least bins * sizeof(float complex) of input to be able
    // to act on the input.
    //qsSetInputThreshold(0, batchSize);
    qsSetInputReadPromise(0, batchSize);

    // We promise not to write more than maxWrite of output.
    qsCreateOutputBuffer(0/*out port 0*/, maxWrite);

    return 0; // success
}


int stop(uint32_t numInPorts, uint32_t numOutPorts) {

    fftwf_destroy_plan(plan);
    return 0;
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    size_t len = lens[0];
    if(len < batchSize) {
        // We do not have enough data to act.
        //DSPEW("Not enough data in len=%zu maxWrite=%zu batchSize=%zu",
        //        len, maxWrite, batchSize);
        return 0;
    }

    if(len > maxWrite)
        len = maxWrite;
    else
        // len must be a multiple of batchSize, and
        // not larger than lens[0] (input length).
        // We know it's at least batchSize or larger.
        len -= (len % batchSize);

    // TODO: Ya, we know how much output we have and so this filter is a
    // good candidate for being a multi-threaded filter.
    // fftw_execute_dft() is also thread-safe, so they say.

    float complex *outBuf = qsGetOutputBuffer(0, maxWrite, maxWrite);
    float complex *inBuf = buffers[0];

    // We have an equal size of input and output.
    // maxWrite is a multiple of batchSize.
    //
    for(size_t n = maxWrite/batchSize; n; --n) {
        fftwf_execute_dft(plan, inBuf, outBuf);
        inBuf += bins;
        outBuf += bins;
    }

    qsAdvanceInput(0/*port*/, len);
    qsOutput(0/*port*/, len);

    return 0; // continue.
}
