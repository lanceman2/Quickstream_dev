# faststream

data flows between modules in a directed graph

## Building and Installing from GitHub Repository Source

Change directory to the top source directory and then run:

    ./bootstrap

Then

    echo "PREFIX = MY_PREFIX" > config.make

where ``MY_PREFIX`` is the top installation directory, like for example
*/usr/local/encap/faststream*.  Then run

    make download

to download additional files to the source directory.
Then run

    make

to generate (compile) files in the source directory.
Then run

    make install

to install files into the prefix directory you defined.


## Description

It's like UNIX streams but much faster.  Modules in the stream, or
pipe-line, may be separate processes, or threads in the same process.  The
stream can be a general directed graph without cycles.

It's faster because the data flowing between the modules is passed through
shared memory and not a kernel buffer, as in UNIX file streams.  In a UNIX
file stream, or a UNIX pipe-line, in order to get data from one program
(module) to the next program the data is transferred to a buffer that the
kernel must write on behave of the program, costing a mode switch, and
then the reading program must have the kernel read the buffer on its
behave, costing another mode switch.  In addition to these two kernel
(system) calls the data is copied twice, once to the kernel buffer for the
writer and once from the kernel buffer to the reader processes buffer.
Comparing these resource costs with passing data via shared memory for the
non-contentious case there is no data copying, and there is no kernel, or
system call, and so we expect it to be much faster.  We expect that a
contentious case to be infrequent and still much faster than the UNIX
pipe-line when running in its normal mode.  We also can eliminate most
contentious cases by using a lock-less circular buffer, where each
producer and consumer module makes promises as to how they will access the
circular buffer in order to guarantee consistent lock-less operation.

faststream is minimalistic and generic.  It is software not designed for
a particular use case.  It is intended to introduce a software design
pattern more so than a particular software development frame-work; as
such, it may be used as a basis to build a frame-work to write programs to
process audio, video, software defined radio (SDR), or any kind of digit
flow pipe-line.

Interfaces in faststream are minimalistic.  To make a filter you do not
necessarily need to consider data flow connection types.  Connection types
are left in the faststream user domain.  In the same way that UNIX
pipe-lines don't care what type of data is flowing in the pipe,
faststream just provides a data streaming utility.  So if to put the wrong
type of data into your pipe-line, you get what you pay for.  The API
(application programming interface) is also minimalistic, in that you do
not need to waste time figuring out what functions and/or classes to use
to do a particular task.

The intent is to construct a flow stream of filters.  The filters do not
necessarily concern themselves with their neighboring filters; the filters
just read input from input channels and write output to output channels,
not necessarily knowing what is writing to them or what is reading from
them; at least that is the mode of operation at this protocol
(faststream API) level.  The user may add more structure to that if they
need to.  It's like the other UNIX abstractions like sockets, in that the
type of data is of no concern in this faststream APIs.


## Terminology

### Stream
The directed graph that data flows in.  

#### Stream states
A high level view of the stream state diagram (not to be confused with a
stream flow directed graph) looks like so:

![image of stream simple state](https://raw.githubusercontent.com/lanceman2/faststream.doc/master/stateSimple.png)

which is what a high level user will see.  It's like a very simple video
player with the video already selected.  A high level user will not see
the transition though the *pause* on the way to exit, but it is there.

If we wish to see more programming detail the stream state diagram can be
explained to this:

![image of stream expanded state](https://raw.githubusercontent.com/lanceman2/faststream.doc/master/stateExpaned.png)

A faststream filter writer will need to understand the expanded state
diagram.  Because there can be more than one filter there must be
intermediate transition states between a *paused* and flowing state, so
that the filters may learn about their connectivity before flowing, and
so that the filter may shutdown if that is needed.

There is only one thread running except in the flow and flush
state.  The flush state is no different than the flow state
except that sources are no longer being feed.

- **wait** is a true do nothing state, a process in paused waiting for
  user input.  This state may skipped through without waiting if the
  program running non-interactively.  This state may be is needed for
  interactive programs.
- **configure** While in this mode the stream can be configured.  All the
  filter modules are loaded, filter modules *construct* functions are
  called, and stream filter connections are figured out.  The thread and
  process repartitioning can only happen in the *configure* state.  Filter
  modules can also have their *destroy* functions called and they can be
  unloaded.  Reasoning: *construct* functions may be required to be called
  so that constraints that only known by the filter are shown to the user,
  so that the user can construct the stream.  The alternative would be to
  have particular filter connection constrains exposed separately from
  outside the module dynamic share object (DSO), which would make filter
  module management a much larger thing, like most other stream toolkits.
  We keep it simple at the faststream programming level.
- **start**: the filters start functions are called. filters will see how
  many input and output channels they will have just before they start
  running.  There is only one thread running.  No data is flowing in the
  stream when it is in the start state.
- **flow**: the filters feed each other and the number of threads and
  processes running depends on the thread and process partitioning scheme
  that is being used.  A long running program that keeps busy will pend
  most of its time in the *flow* state.
- **flush**: stream sources are no longer being feed, and all other
  non-source filters are running until all input the data drys up.
- **stop**: the filters stop functions are being called. There is only one
  thread running.  No data is flowing in the stream when it is in the
  stop state.
- **destroy** in reverse load order, all remaining filter modules
  *destroy* functions are called, if they exist, and then they are
  unload.
- **return**: no processes are running.

The stream may cycle through the flow state loop any number of times,
depending on your use, or it may not go through the flow state at all,
and go from pause to exit.


### Filter

A module reads input and writes outputs in the stream.  The number of
input channels and the number of output channels may be from zero to any
integer.  filters do not know if they are running as a single thread by
themselves or they are sharing their thread execution with other filters.
From the faststream user filters just provide executable object code.

### Controller

Or filter controller: That which changes the behavior of a filter
module.

### Monitor

Or filter monitor: That which monitors a filter, without changing how it
processes inputs and writes outputs.

### Channel

Channel is a connection to a filter from another filter.  For a given
filter running in a stream at a particular time, there are input channels
and output channels.  For that filter running in a given cycle the input
and output channels are numbered from 0 to N-1, where N is the number of
input or output channels.

### Source

A filter with no inputs is a source filter.  It may get "input" from
something other than the stream, like a file, or a socket, but those
kinds of inputs are external from the faststream, that is they do get
any input from the faststream circular buffer.  So in a more global sense
that may not be sources, but with respect to faststream they are sources.

### Sink

A filter with no outputs is a sink filter.  It may write "output" to
something other than the stream, like a file, a socket, or display device,
but those kinds of outputs are external from the faststream, that is they
do not read a faststream circular buffer using the faststream API.  In a
more global sense that may not be sinks, but with respect to faststream
they are sinks.


## Interfaces

faststream has two APIs (application programming interfaces) and some
utility programs:

- a filter API **libfsf**: which is used to build a faststream filter
  dynamic shared object filter modules.
- a stream program API **libfs**: which is used to build programs that run
  a faststream with said filters.
- the program **fsrun**: which uses *libfs* to run a faststream with said filters

## OS (operating system) Ports

Debian 9 and maybe Ubuntu 18.04


## Prerequisite packages
- graphviz
- graphviz-dev
- imagemagick
- make

## Similar Software Projects

Most other stream flow like software projects have a much more particular
scope of usage than faststream.  These software packages to simular
things.  We study them and learn.

- **GNUradio** https://www.gnuradio.org/
- **gstreamer** https://gstreamer.freedesktop.org/
- **RedHawk** https://geontech.com/redhawk-sdr/
- **csdr** https://github.com/simonyiszk/csdr I like the way this uses
  UNIX pipe-line stream to make its flow stream.

### What faststream intends to do better

Run faster with less system resources.  It's simpler.

In principle it should be able to run fast.  We have no controlling task
manager thread that synchronizes the running processes and/or threads.
All filters keep processing input until they are blocked by a slower
down-stream filter (clogged), or they are waiting for input from an
up-stream filter (throttled).

Simple filters can share the same thread, and whereby use less system
resources, and thereby run the stream faster by avoiding thread context
switches using less time and memory than running filters in separate
threads (or processes).  Shared memory is clearly the fastest inter-thread
and inter-process communication mechanism, and we use shared memory with a
consistent lock-less circular buffer.  Memory is not required to be copied
between filters and can be modified and passed through filters.  Then the
stream is in the flow state there are no systems call, except those that a
filter may introduce, and memory copies across from one filter to another
can be completely avoided using a pass-through buffer.

It's simple.  There is no learning curve.  There's just less there.  There
is only one interface for a given functionality.

The stream may repartition its process and threading scheme at launch-time
and run-time based on stream flow measures, and so it can be adaptive and
can be programmed to be self optimizing at run-time.  Most of the other
stream frame-works just can't do that, they only have one thread and
process running scheme that is hard coded, like one thread per filter, or
one process per filter, which of a very simple filter makes no sense.

You can change the filter stream topology on the fly.  Loading and
unloading filters and reconnecting filters at run-time.

In the future benchmarking will tell.  TODO: Add links here...

## A Typical faststream Flow Graph

We introduce terms in this figure:

![image of stream state](https://raw.githubusercontent.com/lanceman2/faststream.doc/master/fastStream_tfg.png)



