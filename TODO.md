This file is just used for quickstream design discussion in a free format,
given that quickstream is not in an alpha (usable) state yet.  Who cares.


# Current Thoughts

 Keep the scheduler code modular so that we may use different schedulers.

 In the process of developing Python integration we have found that the
 basic structure of the code is lacking core elements.   Looks like
 the controllers need to run in threads with the filter (filter-blocks).
 Should we adopt GNUradio terminology?

  GNUradio notes:

 Typing the stream data sucks.

 Q: Is the block input/output typing making stupid things like stream tags
 in order to add more information to each frame?  One could just add a bit
 to the sample frame size and get one bit of information added to each
 sample, but because the types limit that frame size we are pushed to
 adding stream tags to some frames.   In this respect stream ports types
 force their to be a new feature, stream tags, which adds complexity.  We
 need to minimize features, which is to minimize complexity.  Stream tags
 do not need to exist if we do not restrict stream port types.

 
 Make sure that all modules point to their source: relative file and URL.
 Maybe install source as a reference.



 quickstream needed features:

   We must minimize this list:

 - Triggers and events -> scheduler that is modular
    events trigger worker jobs to queue.
    Is there a seperate job queue for filters and job queue for
    controllers?

   filter-block and controller-blocks that have blocking calls will
   work, but may block the flow, when there are fewer worker threads.


 - super modules for combinations of filters and controllers.
 - filter-blocks
 - controller-blocks
 - stream data and buffering
 - parameter setting and getting
     * fixed vs. changing
       - whither a parameter is fixed or changing is determined at
         start time.
       - if a filter or controller does not allow a parameter to change
         may be determined at start-time.

 - fixed vs. run-time (flow-time) changing parameters.
    * this would remove need for contruct(int argc, const char **args) 
        which just set parameters that were being initialized


 - We need interfaces to create and quickstream program's structure,
   other than just a text editor:
   We need hooks/interfaces into all the structures of quickstream
   * display (read only): of quickstream program's structure
     - We can currently see the dot graph of the filter streams.
   * edit and display (r/w): edits and displays quickstream program's structure
     - This is like the GNUradio-companion
   * edit (w): creates a quickstream program with no (or minimal) display



Extending quickstream to multiple processes.

   * make a source and sink filter that reads an inter-process shared
     memory ring buffer.
   * make a controller that uses a inter-process shared memory paramter
     queue thingy.

   Putting these filters and controllers in the stream graphs then expends
   the quickstream program to mutipule processes.  How do you start it?




 - make source filters that have inputs?  "Self-triggered filters" ?

   example:  If an input is not happining, but the filter needs to make
     an output that is not causes by any of its' inputs.

   2 possible Solutions:

       The winner solution needs to minimize system resourses at
       flow-time and minimize the number of quickstream concepts.

       1. A dummy source filter feeds it.  The dummy filter would have a
          zero size ring buffer as an output.
       2. Mark the filter as a "source-like" filter.



 Q: Should the quickstream graph connections be changeable at flow time?

    Or is it inherit to the stream flow module that the graphs not change
    at flow-time?  Maybe we can have the flow-state not be a stream wide
    construct, and so than we could edit the modules that are not running
    (flowing) while other non-neighbor modules are running.
    I expect idea this has a fatal flaw in it.


 - Need to add an optional flush() function to the filters
   (filter-blocks), and remove the isFlushing bool arg from the input().



 We have a directed graph of filter blocks with e


 Q: Does GUNradio have the idea of running controller code?

    A: Best guess is no, not explicitly.  But one could have the
    controller be in main thread.  GNUradio does not provide a module
    construct that is a controller, you need to build that yourself.

 Q: should the filters (blocks) run with the same worker thread job queue
    thingy as controller run with?


 Fundamental redesign.  It's turning into a JavaScript Engine.  It needs
 to be event driven, and async coding.  Triggers cause events.  If there
 is a blocking call than that Controller or Filter (block) will eat a
 worker thread.  That's okay, but could make running with 0 or 1 worker
 thread very slow.
 

 Scoping template code sucks.  Scoping template C++ code is pretty much
 impossible.  Scoping wrapper code sucks.  Simple libc C code is easy to
 follow.  Grepping for pthread_create() is easy, but wrappers can make
 that not possible.  Keep code fucking simple.  Do not use boost.


# Next


- Python integration

  - python controller module that loads .py files.
    The quickstream program would indirectly load .py controller files
    by using the python.so controller.  The python.so would be a proxey
    for all the .py controller modules and a given App.
  - python filter module  The quickstream program would indirectly load
    .py filter files via the wrapper filter module python.so.  There would
    be a one python.so filter loaded per .py each filter, unlike the
    python controllers.

- passing FILE to python controller help() function.  Currently is just
  assumes that file is stdout using print.  This will always work for the
  current quickstream binary, but the API let you use FILE being any
  (libc) stream file.

- What does full duplex look like in this "stream paradigm".


- finish adding quickbuild optional package checks in all Makefile files.
  - See list of dependences in README.md

- alsa sound filter sink (play).

- and alsa record (source) filter.


- benchmark program that runs with quickstream and GNU radio and directly
  together and separately; having them run side by side and connected to
  each other.


- Add a neighboring filter mutex locking scheme:
  - To cut down on inter-thread mutex lock contention
    when there are a lot of worker threads.
  - There would be two locks held between two filters when
    in the job scheduling code:  flow.c:RunInput()
  - maybe add it as another flow/run option.

- measure stream mutex lock contention.
  - See if that is adding CPU usage when there are more threads.

- X3DOM spectrum display.  May keep me from getting fired.
  - use RTL-SDR ubs dongle.
    - write I/Q source filter
    - write fft filter
    - write web TCP/IP controller
    - write web TCP/IP sink
    - write node JS server
      - node controller server
      - web launcher
      - stream feed
    - client test web page

- BUG??  if a filters output length is dependent on the input size
  how can is be sure that it will not output too much?  Answer:
  The filter must reframe from reading all the input, if the input
  is too long.


- Write a test that would fail if quickstream did not continue calling
  input() because input was not advanced because a implicit threshold
  condition is not reached.  See comments in flow.c in function RunInput()
  near: line 243: bool inputAdvanced = false the b) clause was not
  implemented and it was a hard BUG to find.


- Fix the barfing of bash autocompletion due to the filter and
  controller options in { }.


- Change the filter function name from input() to work().
  This is a BIG edit every file and every comment/doc like change.
  Reasons for using input():
    - the word input() describes the main action of what's going on.
      The word input() implies flow.
    - the word work() is secondary compared to the word input().  Work is
      not a required thing that needs to happen for this function.  Flow
      is required more so than computation.  Compution (work) is not
      required at all, but flow and therefore input() are required.
    - it's already using input() in the current source code.
  Reason for using work():
    - work() is standard in GNU radio.  GNU radio is the current leader in
      SDR.  As a result that eases adoption of quickstream.



- Add mention of Parameters to README.md

- Need some kind to controller::preStart() call ordering
  otherwise they can't get called after preStart() in
  other controllers.  See tests/391_controller_bytesCount.

- make userData const in qsParameter*() functions.

- Add idea of parameter to README.md

- Add App function qsAppControllerRequire().  Crap!  How does that work?

- Finish parameters owned by the controller.  And add a test (tests/).

- Super modules.  Super modules are modules that load modules.

- Code for handling module dependences??? For both filters and
  controllers.
  - This may be a major project.
  - How do super modules fix with this idea?

- Q: Is input length --> preInput and  output length --> postInput and
  isFlushing the only paramenter to the controller callbacks?
  They get the callback parameters quickstream API Parmeters, and what
  ever 

- make the parameter thing for all filters that monitors the total number
  of bytes out, and in, for each port and total for all inputs and
  outputs.   Use qsParameterCreateForFilter().

- finish the isFlushing thing:
  - is it necessary
  - add a test for isFlushing.


- Fix Usage: in the module help() thingy.  Maybe automate it more.

- Add URLs to module help() spew.  So that means that each module has
  a corresponding web page on a web site; on Github.com we suppose.


- Make the repo and website dependency of this software project be
  configurable.  Some how.  This is a hard problem; we do not want this to
  add to user configuration complexity.  Example take github.com out of
  all source files but one.  Should MicroSoft decide to destroy github
  we need to ready to move this software project to a new home.  Current
  github policy works for now, so we think.


- Go workout.  Strong body = strong mind.


- Add file URLs to bash completion help in
  share/bash-completion/completions/quickstream
  - So the user can type "quickstream --stuff <tab><tab>" and get
    HTML help links in the bash completion spew.
  - So we get many cross linkings of documentation in quickstream
  - Both local files and on the web...

- Add documention cross linking to the DSPEW(), INFO() and so on CPP
  macros.
  - For the binary quickstream.
  - For libquickstream.so

- Add tty color to the DSPEW(), INFO(), NOTICE(), ... class.

- An --color option to program "quickstream".


- Add suffix and prefix to help() spew to controller modules.

- Add suffix and prefix to help() spew to filter modules.

- controller webClient and the quickstream web server.
  - We can pass process virtual addresses along with name strings and from
    the client and web server.  This would make a very fast lookup to find
    the parameter data that is in the running stream.

- Add controllers to the DOT printing of the streams.

- Write a test (in tests/) that has a filter with more than one input
  channel/port where one of the input channels is slow compared to the
  others.  The filter reading this slow channel syncs the slow input
  channel with faster input channel combining the inputs and writing one
  or more outputs from the combining the inputs.
    - Do that with the output being a pass-through buffer from one of the
      inputs channels.  The output channel could be something like the
      sum of two floats, one from each of the two input channels.

- Write some memory leak tests.  Tests that run and restart enough so as
  to show a memory leak.  Maybe valgrind could help.

- Add finding controller modules to bash completion.

- Super filter that loads other filters.

- Edit documentation from top to bottom.


- Write a master release test that must be run and passed in order to
  make different versions of "releases".
  - Add git download and build/install test.
  - Add tarball generation and build/install test.
  - Test documentation generation both local and on the web.
  - Build/Install with quickbuild
  - Build/Install with GNU autotools
  - Run tests from the built source
  - Run the tests from installed prefix
  - Run tests with all the above permutations.
    Examples:
    - With source dir build from tarball
    - With installed prefix from git repo
  - Keep quickstream self contained so that the documentation
    comes with the source, in one repo.
    - The generated documentation gets pushed to the web.
  - It may be a good idea to minimize the dependency on github
    so that we can port all the source to other repo servers.
    - Hence minimize the use of the github web API, or
      at least make it easy to spot and change code that
      uses that web API.  Release tests should exercise any
      web APIs that are used.  It's called foresite...


- Do we want to be able to remove individual parameters?

- Add a Controller and filter unload test.

- Make GNU autotools build work again.

- Control objects either set or get.  Pairs of control objects like for
  example tx:freq has a setter and a getter; with the getter can be used
  after the setter to see what the setter did.  The setter/getter combo
  can to be synchronous and asynchronous.  So that gives 3 functions:
  get(), set(withCallback), which are both non-blocking and just do what
  they can, and the optional set::callback is called after the action
  completes, and a setSync() that is blocking and waits to get what the
  result is after the set() part; and the get() should not need a
  callback, it is required to store the current state.  The set() and
  setSync() functions do not necessarily keep state.  ... But wait there's
  one more:  what if a controller needs all state changes: than we have
  getCallback(callback) which registers interest in parameter state
  changes, the get callback is called every time the parameter changes.
  If a set happens and does not cause a change to the get state, then the
  get callbacks are not called.  setSync() may not be a good thing to call
  in a filter::input(), given that it blocks.

  The rate at which get() and set() calls occur must be much slower than
  the rate at which data flows in the stream.  We mean that as a general
  principle.  If the control parameters change quickly, you should
  consider putting them as channels in the stream.

  In summary two functions times two variants.  set() and get() with and
  without callbacks.  We can set callback=0 for the non-callback versions.
  Only the set() with callback=0 is a blocking call.  The get() without a
  callback is required to be non-blocking, so the get() must keep a
  current state with it.  The set() does not require state.

  But wait there's more: controls may be grouped in sets.  The
  filter/controller makes and serves the so defined groups.  The thing
  (being filter or controller) getting values via get() may need a group
  of parameters all at the same time.  The serving filter/controller must
  keep the values that are being got consistent but putting up values only
  in consistent groups.  If a parameter is part of a group, it is assumed
  that all the values shelved in that group are always consistent; so if
  not all values in a parameter group change, it is assumed that the
  parameters that did not change are consistent with all the parameters
  that did change and all parameters in the group are "pushed" to their
  registered getters.  The getters need to get() all parameters that it
  wishes to stay consistent in one get() call.

  Simple control group solution: have the producers of said groups makes
  the groups as a composition of the parameters internally.  So a user
  of a group uses the same interface as the user of a single parameter;
  the group parameter is just a grouping of smaller parameters.

  Maybe automatically make all parameters from a given controller or
  filter, be in a group with the name of that controller or filter.

  Can a Controller apply controls across multiple stream.  An App is a
  manager of a group of Controllers.


## Future improvements

- See if making the branches in the dictionary with a 1 bit selector at
  and near each branch, making it a binary tree and not a quad tree that
  it currently is.  This may not be an improvement, we need to see if it
  is an improvement.

## References

https://www.gnuradio.org/doc/doxygen/page_operating_fg.html

https://www.gnuradio.org/doc/doxygen/page_msg_passing.html

https://www.gnuradio.org/doc/doxygen/page_stream_tags.html

https://www.bastibl.net/gnuradio-scheduler-1/

https://www.bastibl.net/blog/

Best one:
https://gnss-sdr.org/docs/fundamentals/


Does GNUradio synchronise the data for all input channels when the
filter/block does its work?

So with I/O types then there is a concept of input frames which always
have a fixed size.

Quickstream does not synchronise the data for all input channels.


## Threads and buffer interaction

Can we reduce the thread waits by increase buffer sizes by adding an
additional buffer length that can act as thread recoil that keeps threads
from waiting at the cost of added latency.  The read pointers would not
"hit" the corresponding write pointers so often.  Think of the read
pointers being pulled behind the write pointer at an equilibrium distance
from it, with some damped oscillation about that distance.

The distances between the read pointers and the corresponding write
pointer needs to be measured as a function time, or some kind of counter
that acts like time; given time is too expensive to measure.

There may be a good analogy between rail car and couplers, and the running
threads and buffers.  If coupling between two cars have to hard stops for
the two positions, one when we push the cars together and the other when
we pull the two cars apart.  In between these to hard stops we have
buffering space.  If we added a spring and damping in the coupler the two
hard stops become softened.

For the single thread run case this would not help.



## Kahn’s process network

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

No runtime scheduler is needed the threads just control themselves.  It's
less complex.  If bottleneck filters are thread-safe this will be able to
process data faster than GNUradio by having multithreaded filters.



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


