
This file just used for quickstream design discussion in a free format,
given that quickstream is not in an alpha (usable) state yet.


## Add more automatic buffer sizing.

These example cases will not happen but in a couple stream flow cycles.

A qsGetBuffer() call with a length that would overflow the buffer, will
cause the buffer to increase its' size.

For there to be lock-less ring buffers the writing filter must not
change the content of the old buffer as it copies it to a new larger
ring buffer.  When we are sure that all filters that may be reading
the old ring buffer are finished with the old ring buffer, it may be
freed (removed/unmapped/whatever).

Pass-through buffers may not be possible with lock-less ring buffers and
the automatic buffer sizing feature.

Next Question: How do we know when we can remove "old-resized" ring
buffers?

  CASES:

    * All filters reading the buffer are in the same thread as the
      filter that is writing (owns) the ring buffer.  Then just
      remove it at the time the new ring buffer is created.

    * For a given ring buffer, some reading filters may be in another
      thread.


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
determined by the next encountered qsOutput() call.   The order of
qsOutput() calls will determine the order of filter input() calls.  When
qsOutput() is called it will not call the next filter input() in the
function call stack, but will queue it up and call the next filter inptu()
after the current filter input() function returns.  This way will require
simpler thread synchronization, because we know that threads will only
access one full input() call at a time, and there will not be a stack of
input() functions.

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

When data in the stream is flowing at a regular rate.  It's a real-time
stream.

In the limit of fast processing speed will make the latency be the input()
length divided by the data flow rate.  So small input() lengths will give
small latencies.  The smallest processable input() length will give the
smallest latency at a cost of having more input() calls per unit of data
output.


## Filter options and parameters at *construct()* and *start()*

Add generic option thingy that manages filter options.  It needs to keep
filter interface requirements to a minimum.  It should not require any
filter writer user coding, if they are just using the built in default
filter options.

  1. This needs to automatically add the options to higher level
     interfaces, without the filter writer user considering what the
     option interfaces are.  So construct(argc, argv) may not be what we
     want.

  2. Two types of options: *construct()* and *start()*.

     * *construct()* options take effect at filter *construct()*.  So
       parameters changed by these options just happen when the filter
       is loaded.

     * *start()* options take effect at filter *start()*.  So parameters
       can be changed between flow cycles, and not just at filter load
       time.



