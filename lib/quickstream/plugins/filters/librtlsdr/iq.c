#include <inttypes.h>
#include <rtl-sdr.h>

#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"


// From: https://www.rtl-sdr.com/about-rtl-sdr/
//
// The maximum sample rate is 3.2 MS/s (mega samples per second). However,
// the RTL-SDR is unstable at this rate and may drop samples. The maximum
// sample rate that does not drop samples is 2.56 MS/s, however some
// people have had luck with 2.8MS/s and 3.2 MS/s working well on some USB
// 3.0 ports.
//
// By running:
// quickstream -C bytesCounter -t 0 -f librtlsdr/iq -f nullSink -c -r
// we managed to run with no read error with rate = 3200000
// and just 0.7 to 1.3 % CPU usage on one thread and 0 on other.

//#define DEFAULT_RATE   ((uint32_t) 2048000)
//
#define DEFAULT_RATE   ((uint32_t)  3200000)
#define DEFAULT_GAIN   ((int) -1)
#define DEFAULT_FREQ   ((uint32_t) 105300000)

#define DEFAULT_NUM    ((size_t) 0) // 0 = infinite

#define DEFAULT_MAXWRITE ((size_t) 8*1024)

//example:
//Time to read a buffer of this size = 8*1024/3200000 = 0.00256 sec


void help(FILE *f) {

    fprintf(f,

"  Usage: iq\n"
"\n"
"  This filter is a source with one output port.\n"
"  This filter must have 0 inputs.\n"
"  This filter will only open one RTL-SDR device.\n"
"\n"
"  NOTES:\n"
"\n"
"  This filter uses librtlsdr which launches another thread.  As\n"
"  expected that thread they launch, for the most part, does nothing but\n"
"  sleep.  Hence I do not like librtlsdr and libusb.\n"
"  I think the sleeping thread spews to stderr if there are errors.\n"
"  I'm guessing it's some kind of monitor thread.\n"
"\n"
"  Adjusting --maxWrite can change CPU usage considerably.\n"
"\n"
"\n"
"\n"
"                     OPTIONS\n"
"\n"
"    --freq HZ  set the dongle center frequency to HZ Hz.  The default\n"
"               center frequency is %" PRIu32 " Hz.\n"
"\n"
"\n"
"    --gain DB  set the dongle gain to DB dB.  The default gain is\n"
"               %d dB.  We use -1 is for auto gain mode.\n"
"\n"
"\n"
"    --maxWrite BYTES  default value %zu.  This is the maximum number of\n"
"                      written for each input() call.\n"
"\n"
"\n"
"    --num N   number of bytes to read per stream run.  The default is\n"
"              0 for infinite.  This value will get rounded up to the\n"
"              nearest multiple of maxWrite BYTES, due to a limitation\n"
"              of the rtl-sdr dongle or librtlsdr.\n"
"\n"
"\n"
"    --rate HZ  set the dongle sample rate to HZ Hz.  The default sample\n"
"               rate is %" PRIu32 " Hz.\n"
"\n"
"\n",
    DEFAULT_FREQ, DEFAULT_GAIN,
    DEFAULT_MAXWRITE,
    DEFAULT_RATE
        );
}



static rtlsdr_dev_t *dev = 0;

// quickstream Parameters
//
static
uint32_t rate; // in Hz. used in rtlsdr_set_sample_rate()
//
static
uint32_t freq; // in Hz.  used in rtlsdr_set_center_freq()
//
static
int gain; // in dB. -1 -> auto gain.  used in rtlsdr_set_tuner_gain()

static
size_t num; // number bytes to read from device.
static
size_t total; // number bytes currently read.

static
size_t maxWrite; // bytes.  Max we'll output per input() call.


// TODO: add a mutex for remote parameter setting and getting.

// TODO: We may need to use rtlsdr_set_freq_correction()


static inline
int setRate(uint32_t rate) {

    int r = rtlsdr_set_sample_rate(dev, rate);
    DASSERT(r <= 0);
    if(r < 0) {
        ERROR("rtlsdr_set_sample_rate(,%" PRIu32 ") failed",
                rate);
        return -2; // error
    }
    return 0; //success
}


static inline
int setFreq(uint32_t freq) {

    int r = rtlsdr_set_center_freq(dev, freq);
    DASSERT(r <= 0);
    if(r < 0) {
        ERROR("rtlsdr_set_center_freq(,%" PRIu32 ") failed",
                freq);
        return -3; // error
    }
    return 0; //success
}


static inline
int setGain(int gain) {

    int r;
    int mode = 1; //manual gain mode

    if(gain < 0)
        mode = 0; //automatic gain mode.

    r = rtlsdr_set_tuner_gain_mode(dev, mode);
    if(r != 0) {
        ERROR("rtlsdr_set_tuner_gain_mode(,%d) failed",
                mode);
        return -4;
    }
    if(mode == 0)
        // automatic mode
        return 0; //success

    // Manual mode.
    r = rtlsdr_set_tuner_gain(dev, gain);
    if(r != 0) {
        ERROR("rtlsdr_set_tuner_gain(,%d) failed",
                gain);
        return -5;
    }
    return 0; //success
}



int construct(int argc, const char **argv) {

    rate = qsOptsGetUint32(argc, argv, "rate", DEFAULT_RATE);
    gain = qsOptsGetInt(argc, argv, "gain", DEFAULT_GAIN);
    freq = qsOptsGetUint32(argc, argv, "freq", DEFAULT_FREQ);
    num = qsOptsGetSizeT(argc, argv, "num", DEFAULT_NUM);
    maxWrite = qsOptsGetSizeT(argc, argv, "maxWrite", DEFAULT_MAXWRITE);

    if(maxWrite < 1023) {
        ERROR("maxWrite (%zu) is too small", maxWrite);
        return -1;
    }

    if(num && num % maxWrite)
        num +=  maxWrite - (num % maxWrite);

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 0);
    ASSERT(numOutPorts == 1);

    DASSERT(dev == 0);

    int r = rtlsdr_open(&dev, 0);

    if(r != 0) {
        ERROR("rtlsdr_open() failed");
        return -1; // error
    }

    if((r = setRate(rate))) return r;
    if((r = setFreq(freq))) return r;
    if((r = setGain(gain))) return r;

    if((r = rtlsdr_reset_buffer(dev))) {
        ERROR("rtlsdr_reset_buffer() failed");
        return -6;
    }

    total = 0; // bytes

    qsCreateOutputBuffer(0/*output port*/, maxWrite/*maxWriteLen*/);

    return 0; // success
}

int stop(uint32_t numInPorts, uint32_t numOutPorts) {

    if(dev) {
        rtlsdr_close(dev);
        dev = 0;
    }

    return 0;
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    // Bull shit interface uses an int for the length read.
    int lenRead = 0;

    size_t lenRequest = maxWrite;
    if(num && lenRequest + total > num)
        lenRequest = num - total;

    // TODO: If we always do read lenRequest we can set min to lenRequest.
    // But that only helps if we could make this filter multi-threaded,
    // and it's not likely the librtlsdr code could do that.
    void *outBuf = qsGetOutputBuffer(0, lenRequest, 0/*min*/);

    // I'm assuming the lenRead is the number of bytes read.
    int r = rtlsdr_read_sync(dev, outBuf, lenRequest, &lenRead);

    if(r < 0) {
        ERROR("rtlsdr_read_sync() failed");
        return -1; // fail/bail
    }

    if(lenRead > 0) {
        if(lenRead != lenRequest) {
            // Just testing:
            // This never seems to happen.
            ERROR();
        }
        qsOutput(0/*port*/, lenRead);
    } else {
        // I have not seen this happen yet.
        ERROR("rtlsdr_read_sync() fucked up");
        return -2;
    }

    total += lenRead;

    DASSERT(num == 0 || total <= num);

    if(total == num)
        // finished running.
        return 1;


    return 0; // continue.
}
