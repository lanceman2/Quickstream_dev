# faststream
data flows between modules in a directed graph

It's like UNIX streams but much faster.  Modules in the stream, or
pipe-line, may be separate processes, or threads in the same process.  The
stream can be a general directed graph.

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
type of data is of no concern in this faststream API.

## Taxonomy

### Stream
The directed graph that data flows in.  

A stream state diagram (not to be confused with a stream flow directed
graph) looks like so:
![image of stream state](../faststream.doc/master/fastStream_states.png)


#### Stream states

There is only one thread running except in the running and flushing
state.  The flushing state is no different than the running state
except that sources are no longer being feed.

- paused: any faststream must begin in a paused state.  filters
  constructors and destructors are called and connections between filters
  may be edited, and the topology of the streams may be changed when it is
  in the paused state.  There is only one thread running in the paused
  state.  The paused state is the stream configuring state.  The thread
  and process repartitioning can only happen in the paused state.
- starting: the filters start functions are called. filters will see how
  many input and output channels they will have just before they start
  running.  There is only one thread running.  No data is flowing in the
  stream when it is in the starting state.
- running: the filters feed each other and the number of threads and
  processes running depends on the thread and process partitioning scheme
  that is being used.
- flushing: stream sources are no longer being feed, and all other
  non-source filters are running until all input the data drys up.
- stopping: the filters stop functions are being called. There is only one
  thread running.  No data is flowing in the stream when it is in the
  stopping state.
- exit: no processes are running.

The stream may cycle through the running state loop any number of times,
depending on your use, or it may not go through the running state at all,
and go from paused to exit.

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


## OS (operating system) Ports
Debian 9 and maybe Ubuntu 18.04


## Prerequisite packages
- graphviz
- graphviz-dev
- imagemagick

