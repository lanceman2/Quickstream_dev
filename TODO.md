
This file just used for quickstream design discussion in a free format,
given that quickstream is not in an alpha (usable) state yet.


## References

https://www.gnuradio.org/doc/doxygen/page_operating_fg.html

https://www.gnuradio.org/doc/doxygen/page_msg_passing.html

https://www.gnuradio.org/doc/doxygen/page_stream_tags.html

https://www.bastibl.net/gnuradio-scheduler-1/

https://www.bastibl.net/blog/

Best one:
https://gnss-sdr.org/docs/fundamentals/


## Not a Kahn’s process network

At least not like the GNUradio version of one.

https://en.wikipedia.org/wiki/Kahn_process_networks

https://en.wikipedia.org/wiki/Flow-based_programming

We can spread threads across more than one filter module for filter
modules that do not require so much computation.  By letting the thread
run through the stream graph we can have the threads automatically
distribute themselves in an optimal fashion.  Threads are analogous to
migrant laborers, they just go where work is needed.

What is optimized?  In all of nature, physics, something is always
optimized. ???  It'd be nice to architect the software based on an
optimization principle.  It'd provide a nice theme for publishing a nice
white paper.

Bottlenecks will accumulate, block, threads at there input() calls.
Thread-safe filters input() functions may have more than one thread
running.  Non-bottleneck filters will not block threads at there input()
calls.

Many quick filter modules can share one thread.  Slow filters can use more
than one thread at a time.  Net result, it can be faster than a Kahn’s
process network.  It will only be as fast as the bottlenecks be it a
Kahn’s process network or this trans filter threads thing.

No runtime scheduler is needed the thread just control themselves.  It's
less complex.  If bottleneck filters are thread-safe this will be able to
process data faster than GNUradio.



## Feature: reuse ring buffers between restarts??


## Control


Metadata the is not in the stream channels data, but may be paired with
points in the streams channel data.  Controls are published via filter
name and control/parameter name.  Controls are either setters or getters.
The getters are monitors.  Monitor and control will be lumped in one term,
control.

We must separate controls that are setters and controls that are getters
of parameters.  Example: The setting of frequency in a TX filter can then
pop a getter of the frequency.  With the setter and getter model we then
only have one code that is writing the getter and many codes that are
writing the setter with one reading.  The success of a setter is
determined by monitoring a getting.

Each control can have many keys, so we queue up key value pairs.  Each key
is a parameter, but there may be relations between setters and getters
like for example we may have a "frequency setter" and a "frequency getter"
that are two different control parameters, but they are related, and may
be the same, or not if the frequency to set is rounded (or other) to get
the frequency we get returned by the libUHD.

Use var args function form??
control = qsControlCreate(key0, type0, val0, key1, type1, val1,
        key2, type2, val2, ...);

// Or define a parameter defined in a given filter:
parameter = qsCreateParameter(key, val);
// Set the parameter with:
qsParameterSet(parameter, val);

Q: Can we have parameter change events tagged to a point in the ring
buffer?  Do we need that?

All filters will have built-in controls like totalBytesOut and the per
channel version of that.


### Controller

Controller is that which accesses controls, reading or writing.

Controller can be a filter or not.

Must we have controller DSO modules separate from filters?

How does the controller that is not a filter run.

Controller modules are event driven.  Where as filters are driven by the
flow.  Controller could add events that trigger themselves??


## Multi threaded filters

If a filter module input() function is written in a thread-safe manner, we
should be able to have the same filter input() run in more than one
thread, though we would need to keep the order of the outputs correct, so
the data in the ring buffers stays ordered correctly.


## Flowing threads or threads running wild

In this idea we think of the threads as following the "natural" stream
flow as in the single threaded case, but adding threads to the sources
until the threads are blocked by the source or we reach a number threads
equal to a set hardware or software limit.  Any "bottle-neck and
thread-safe" filters would get more threads working on them.  Even without
any thread-safe filters we will get some parallelization when there are
large number of filters.  Maybe just two filters with give us some
parallelization.

#### Thread running model:

Threads call each filter input() in succession, one at a time, with the
order being source filter first and the next filter input() calls being
determined by the next encountered qsOutput() call, that reaches the next
filters needed threshold.  The order of qsOutput() calls will determine
the order of filter input() calls.  When qsOutput() is called it will not
call the next filter input() in the function call stack, but will queue it
up and call the next filter input() after the current filter input()
function returns.  This way will require simpler thread synchronization,
because we know that threads will only access one full input() call at a
time, and there will not be a stack of input() functions.

For multi-threaded filters qsGetBuffer() and qsOutput() calls are both a
potential point of thread synchronization: We can't let the thread that is
behind it call qsGetBuffer() before the thread in front.  The qsOutput()
calls would be a potential point of thread synchronization, because we
can't have a newer thread overtake an older thread in the order of the
flow, otherwise the ring buffers would get written and read out of
order.

A filter needs a "isThreadSafe" attribute added.

Thread contention cases:

  1. qsOutput() to "not thread-safe" filter with a running thread:
     The output is queued, and the thread goes to the "next source
     filter", or it waits in a thread pool, or it exits if there are
     too manys thread already.
  2. qsOutput() to "thread-safe" filter may be getting ahead of the
     current ring buffer.  The length passed to qsGetBuffer() must match
     the length to qsOutput() or both threads can't run between the
     qsGetBuffer() and qsOutput() calls, or ring buffers may be
     corrupted.  The only contention is between qsGetBuffer() and
     qsOutput() calls on the same buffer.

qsGetBuffer() needs a bool argument added that says whither or not
the length requested is just a maximum or the value that will be passed
to the corresponding qsOutput() call.


## Varying buffer read lengths at flow time

If we have thread-safe filters, and can vary the buffer input() length by
varying maxRead.  We should be able to optimize different performance
measures while varying those quantities and the number of concurrent
threads that run a filter input() function.

We'll assume the filter is letting the quickstream code control the length
of input data that is being processed at each input() call, otherwise
quickstream is not being utilized well.

TODO: We need to have a method to determine when a reallocated ring buffer
can be un-mapped.

In the limit of fast processing speed will make the latency will be the
input() length divided by the data flow rate, for any channel.  So small
input() lengths will give small latencies.  The smallest processable
input() length will give the smallest latency at a cost of having more
input() calls per unit of data output.


## input() with more than one channel

Currently input is called with just one input channel buffer.  We may need
there to be more than one input channel to be read at a time and we may
need corresponding read multi-channel thresholds to be fulfilled in order
for input() to be called.


## Do we need named channels?

So that we can connect filters outputs to the current outputs.


## Filter controls

### Changing filter options and parameters at *construct()* and *start()*

Add generic option thingy that manages filter options.  It needs to keep
filter interface requirements to a minimum.  It should not require any
filter writer user coding, if they are just using the built in default
filter options.

1. This needs to automatically add the options to higher level
   interfaces, without the filter writer user considering what the
   option interfaces are.  So construct(argc, argv) may not be what we
   want.

2. Stage of options: *construct()* and *start()*.

  * *construct()* options take effect at filter *construct()*.  So
    parameters changed by these options just happen when the filter
    is loaded.

  * *start()* options take effect at filter *start()*.  So parameters
    can be changed between flow cycles, and not just at filter load
    time.


### Changing filter options and parameters at flow time


Ya, that too.


### Stream data VS. control data

Stream data is flowing, it is a ordered sequence of bytes.  Tends to have
a constant rate of flow.  There is a fast changing series of values for a
given channel.

Stream data comes in channels.  Channels can have subcomponents like for
example 1 stereo channel can be broken into 2 mono channels.

Control data comes as fixed chunks of data that has values that can be
fixed and vary.  A series of values for a given control is not required.

Control data comes in parts called parameters.

A control parameter can be funneled into, or channeled, into a stream
channel.


