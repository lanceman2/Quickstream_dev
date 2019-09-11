# quickstream

data quickly flows between modules in a directed graph

quickstream is a run library, a filter library, some filter modules, and
some utility programs.  quickstream is written in C and the libraries can
link with C and C++ code.  quickstream is for building flow stream graphs
that process data in filter module stages.

quickstream is the next generation of a stream flow graph API that we
wrote a year or more ago.  The old stream flow graph API was the subset of
larger project, with a larger scope.  I consider quickstream a smaller
more refined spin-off project.  I hope to give quickstream more polish
then it's previous generation.


# Development Status

It's not functional yet.  Currently it's got most of directory structure
laid out, build system, and components figured out.  The thing of interest
now for you may be this README.md file.


## Building and Installing with GNU Autotools

quickstream can be built and installed using a GNU Autotools software
build system.

If you got the source from a repository run

    *./bootstrap*

to add the GNU autotools build files to the source.

  Or if you got this
source code from a software tarball release change directory to the
top source directory and do not run bootstrap.

Then from the top source directory run

  *./configure --prefix=/usr/local/PLACE_TO_INSTALL*

where you replace */usr/local/PLACE_TO_INSTALL* with a directory where you
will like to have the *quickstream* software package installed.  Then run

  *make*

to build the code.  Then run

  *make install*

to install the stream software package in the prefix directory that you
set above.


### Making a Tarball Release

If you wish to remove all files that are generated from the build scripts
in this package, leaving you with just files that are from the repository
and your added costume files you can run *./RepoClean*.  Do not run
*./RepoClean* if you need a clean tarball form of the package source,
use *make distclean* (after running ./configure) for that.

We use the GNU autotool build system to generate tarball releases.  To
make a tarball release, after you finish testing and editing, edit
RELEASE.bash changing the version numbers and so on, then you can run a
sequence of programs something like the following:

  *./RepoClean*

then make sure that the files that remain are what you want, if not you
may need to edit *RepoClean*.  After you're confident that is doing what
you want run:

  *./bootstrap*

which will generate many files needed for a tarball release.  Then run:

  *./configure*

which the is famous GNU autotools generated *configure* script for this
package.  Then run:

  *make dist*

which should generate the tarball file *quickstream-0.0.1.tar.gz* and
other compression format versions of tarball files.


## Building and Installing with quickbuild

The quickstream developers build system.  Without some care the quickbuild
build system and the GNU autotools build system will conflict with each
other.  So why we have two build systems?:

GNU autotools does not allow you to load dynamic shared object (DSO)
plugin modules without installing them first, whereby making a much longer
compile and test cycle.  The same, more-so, can be said of CMake.  As a
code developer we just want to quickly compile source and run it from the
source directory, without installing anything.

The use of quickbuild is for quickstream developers, not users.
quickbuild is just some "GNU make" make files and some bash scripts.  We
added these make files named "Makefile" and other files so that we could
develop, compile and run programs without being forced to install files.
We use "makefile" as the make file for GNU autotools generated make files,
which overrides "Makefile".

The quickbuild make files tend to be much smaller than the GNU autotools
generated make files.  


### quickbuild quick and easy

Run:

  *make*

  *make install PREFIX=MY_PREFIX*

where MY_PREFIX is the top installation directory.  That's it,
if you have the needed dependences it should be installed.


### quickbuild with more options

If you got the quickstream source from a repository, or a tarball,
change directory to the top source directory and then run:

  *./quickbuild*

which will download the file *quickbuild.make*.

Now there are no more downloads needed.  Now run:

  *echo "PREFIX = MY_PREFIX" > config.make*

where ``MY_PREFIX`` is the top installation directory, like for example
*/usr/local/encap/quickstream*.  Then run:

  *make*

to generate (compile) files in the source directory.
Then run:

  *make install*

to install files into the prefix directory you defined.

Note the *PREFIX* will only be used in the *make install* and is not
required to be set until *make install* runs, so alternatively you can
skip making the *config.make* file and in place of running *make install*
you can run *make install PREFIX=MY_PREFIX*.

For debugging and development additional configuration options can be
added to the *config.make* files as C preprocessor flags when compiling
are for example:

  - *CPPFLAGS = -DDEBUG*
  - *CPPFLAGS = -DDEBUG -DSPEW_LEVEL_DEBUG*
  - *CPPFLAGS = -DSPEW_LEVEL_INFO*
  - *CPPFLAGS = "-DDEBUG -DSPEW_LEVEL_NOTICE"*
  - *CPPFLAGS = -DSPEW_LEVEL_WARN*
  - *CFLAGS = -g -Wall -Werror*

See file *lib/debug.h* for how these CPP (C preprocessor) macro flags are
used.

If you wish to remove all files that are generated from the build scripts
in this package, leaving you with just files that are from the repository
and your added costume files you can run *./RepoClean*.  Do not run
*./RepoClean* if you need a clean tarball form of the package source,
use *make distclean* for that.


## quickstream is Generic

Use it to process video, audio, radio signals, or any data flow that
requires fast repetitive data transfer between filter modules.


## quickstream is Fast

The objective is that quickstream flow graph should process data faster
than any other streaming API (application programming interface).


## No connection types

The quickstream use case is generic, in the way it does not care what kind
of data is being transferred between filter stages, in the same way UNIX
pipes don't care what kind of data flows through them.  The types of data
that flows is up to you.  The typing of data flowing between particular
filters is delegated to a higher protocol layer above quickstream.
quickstream provides generic management for the connecting and running of
filter streams.


## Restricting filters modules leads to user control and runtime optimisation

quickstream provides generic management for the connecting and running of
filter streams.  You may change and optimise the distribution of threads
and processes running each of the filters in the stream while your program
is running.  quickstream restricts filter interfaces so that it may
provide an abstraction layer so that filters may be run with their own
thread or run in a inter-filter shared thread as just a function call on
the call stack.  Similarly the distribution of filters across processes
may also be managed at runtime.  This enables you to find what
distribution of threads and processes that is needed to run your stream
fast and use less system resources.


## Description

It's like UNIX streams but much faster.  Filter modules in the stream, or
pipe-line, may be run in separate processes, or separate threads in the
same process, or modules may be run together in a single thread.   The way
processes and threads are distributed across the filter modules in the
running flow stream is not necessarily constrained in any way.  The stream
can be any general directed graph without (or with) cycles.

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

quickstream provides the buffering between module filters in the flow.
The filters may copy the input buffer and then send different data to it's
outputs, or if the output buffers are or the same size as the input
buffers, it may modify an input buffer and send it through to an output
without copying any data were by eliminating an expensive copy
operation.

quickstream is minimalistic and generic.  It is software not designed for
a particular use case.  It is intended to introduce a software design
pattern more so than a particular software development frame-work; as
such, it may be used as a basis to build a frame-work to write programs
to process audio, video, software defined radio (SDR), or any kind of
digit flow pipe-line.

Interfaces in quickstream are minimalistic.  To make a filter you do not
necessarily need to consider data flow connection types.  Connection types
are left in the quickstream user domain.  In the same way that UNIX
pipe-lines don't care what type of data is flowing in the pipe,
quickstream just provides a data streaming utility.  So if you put the
wrong type of data into your pipe-line, you get what you pay for.  The API
(application programming interface) is also minimalistic, in that you do
not need to waste so much time figuring out what functions and/or classes
to use to do a particular task, the choose should be obvious.

The intent is to construct a flow stream of filters.  The filters do not
necessarily concern themselves with their neighboring filters; the filters
just read input from input channels and write output to output channels,
not necessarily knowing what is writing to them or what is reading from
them; at least that is the mode of operation at this protocol (quickstream
API) level.  The user may add more structure to that if they need to.
It's like the other UNIX abstractions like sockets, file streams, and
pipes, in that the type of data is of no concern in this quickstream
APIs.


## Prerequisite packages
Building and installing quickstream requires the following debian package
prerequisites:
- build-essential
- graphviz-dev
- imagemagick
- doxygen


## Terminology

### Stream

The directed graph that data flows in.  


#### Stream states

A high level view of the stream state diagram (not to be confused with a
stream flow directed graph) looks like so:

![image of stream simple state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/stateSimple.png)

which is what a high level user will see.  It's like a very simple video
player with the video already selected.  A high level user will not see
the transition though the *pause* on the way to exit, but it is there.

If we wish to see more programming detail the stream state diagram can be
explained to this:

![image of stream expanded state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/stateExpaned.png)

A quickstream filter writer will need to understand the expanded state
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
  We keep it simple at the quickstream programming level.
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
From the quickstream user filters just provide executable object code.


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
kinds of inputs are external from the quickstream, that is they do get
any input from the quickstream circular buffer.  So in a more global sense
that may not be sources, but with respect to quickstream they are sources.


### Sink

A filter with no outputs is a sink filter.  It may write "output" to
something other than the stream, like a file, a socket, or display device,
but those kinds of outputs are external from the quickstream, that is they
do not read a quickstream circular buffer using the quickstream API.  In a
more global sense that may not be sinks, but with respect to quickstream
they are sinks.


## Interfaces

quickstream has two APIs (application programming interfaces) and some
utility programs.  Both APIs in use the libquickstream library.  The main
parts are:

- a filter API **qsfilter.h**: which is used to build a quickstream filter
  dynamic shared object filter modules.
- a stream program API **qsapp.h**: which is used to build programs that run
  a quickstream with said filters.
- the program **quickstream**: which uses *qsapp.h* to run a quickstream with said
  filters.


## OS (operating system) Ports

Debian 9 and Ubuntu 18.04


## Similar Software Projects

Most other stream flow like software projects have a much more particular
scope of usage than quickstream.  These software packages to similar
things.  We study them and learn.

- **GNUradio** https://www.gnuradio.org/
- **gstreamer** https://gstreamer.freedesktop.org/
- **RedHawk** https://geontech.com/redhawk-sdr/
- **csdr** https://github.com/simonyiszk/csdr We like straight forward way
  csdr uses UNIX pipe-line stream to make its flow stream.


### What quickstream intends to do better

Summary: Run faster with less system resources.  Be simple.

In principle it should be able to run fast.  We have no controlling task
manager thread that synchronizes the running processes and/or threads.
All filters keep processing input until they are blocked by a slower
down-stream filter (clogged), or they are waiting for input from an
up-stream filter (throttled).  The clogging and throttling of the stream
flows is determined to the current inter-filter buffering parameters.

Simple filters can share the same thread, and whereby use less system
resources, and thereby run the stream faster by avoiding thread context
switches using less time and memory than running filters in separate
threads (or processes).  When filter gets more complex we can let the
filter run in it's own thread (or process).  Shared memory is clearly the
fastest inter-thread and inter-process communication mechanism, and we use
shared memory with a consistent lock-less circular buffer.  Memory is not
required to be copied between filters and can be modified and passed
through filters.  Then the stream is in the flow state there are no
systems call, except those that a filter may introduce, and memory copies
across from one filter to another can be completely avoided using a
pass-through buffer.

It's simple.  There is no learning curve.  There's just less there.  There
is only one interface for a given quickstream primitive functionality.

The stream may repartition its process and threading scheme at launch-time
and run-time based on stream flow measures, and so it can be adaptive and
can be programmed to be self optimizing at run-time.  Most of the other
stream frame-works just can't do that, they only have one thread and
process running scheme that is hard coded, like one thread per filter, or
one process per filter, which of a very simple filter makes no sense.

You can change the filter stream topology on the fly.  Loading and
unloading filters and reconnecting filters at run-time.

In the future benchmarking will tell.  TODO: Add links here...


## A Typical quickstream Flow Graph

![simple stream state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/quickstream_simple.png)

![complex stream state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/quickstream_complex.png)


## Developer notes

- quickstream code is written in fairly simple C with very few dependences.
- The API (application programming interface) user sees data structures as
  opaque.  They just know that they are pointers to data structures, and
  they do not see elements in the structures.  We don't use typedef, for
  that extra layer of indirection makes it harder to "scope the code".
- The files in the source directly follow the directory structure of the
  installed files.  Files in the source are located in the same relative
  path of files that they are most directly related to in the installed
  paths.  Consequently:
  - you don't need to wonder where source files are,
  - programs can run in the source directory after running make without
    installing them,
  - the running programs find files from a relative paths between them,
    the same way as in the installed files as with the files in the
    source,
  - we use the compilers relative linking options to link and run
    programs, and
  - you can move the whole encapsulated installation (top install
    directory) and everything runs the same.
- Environment variables allow users to tell quickstream programs where to
  find users files that are not in the quickstream source code.
- The installation prefix directory is not used in an quickstream code,
  only relative paths are needed for running quickstream files to find
  themselves.
- C++ code can link with quickstream.
- The public interfaces are object oriented in a C programming sense.
- The private code is slightly more integrated than the public interfaces
  may suggest.
- We wish to keep this C code small and manageable.
- Minimize the use of CPP macros, so that understanding the code is easy
  and not a CPP macro nightmare like many C and C++ APIs.  Every
  programmer knows that excessive use of CPP macros leads to code that is
  not easily understandable in the future after you haven't looked at it
  in a while.
- If you wish to make a tarball release use the GNU autotool building
  method and run *./configure* and *make dist*.
- Files in the source are located in the same relative path of files that
  they are most directly related to in the installed paths.
