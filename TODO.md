
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

I'd expect that cost of adding to the function call stack in each thread
would be less than the cost of thread synchronization for a thread per
filter case (or any thread bound to filter association).  It may even be
able to run this way without any thread synchronization, hence we call it
threads running wild.  The qsOutput() calls would be a potential point of
thread synchronization, because we can't have a newer thread overtake an
older thread in the order of the flow, otherwise the ring buffers would
get written and read out of order.

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



