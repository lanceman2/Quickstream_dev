# quickstream

data quickly flows between modules in a directed graph


## Development Status

This is currently vapor-ware.

Current development is on Debian 9 and Ubuntu 20.04 systems.  It's in a
pre-alpha state.

Currently there is no tutorial, so this software package is pretty useless
for anything but for the research and development of this package.

We are adopting **some** of the [GNU radio](https://gnuradio.org/) terminology.


## About quickstream


quickstream is a flowgraph framework, a digital signal-processing system
that operates within running processes.  It's internals do not span across
multiple computers, though it's applications may.  quickstream avoids
system calls when switching between processing modules and hence uses
shared memory to pass stream data between modules.

Unlike [GNU radio](https://gnuradio.org/) quickstream does not use a
synchronous data flow model, that is the stream data is not restricted to
keep constant ratios of input to output between blocks. The stream data
flow constraints just less than those imposed by GNUradio.  The flow
scheduler code is not like GNU radio's scheduler.  The run model lets the
user use any number of threads from thread pools which may have 1 to N
threads.  All quickstream programs can run with one thread or any number
of threads.  Thread affinity may be set, if needed, for special blocks.
quickstream is expected to use much less computer system resources than
GNU radio in preforming the similar tasks. 

quickstream is C code with C++ wrappers.  It consists of the quickstream
runtime library and block module plugins.  The quickstream runtime library
and the block module plugins are all created as DSOs (dynamic shared
objects).  quickstream runtime library includes a super block module
creation tool that makes it possible to create and use block module
plugins that are made of many block module plugins.

quickstream comes with two application builder programs:

  - *quickstream* which lets to build flow stream programs using the
            command-line.

  - *quickstreamBuilder* a GUI (graphical user interface) program that
            lets you build and run flow stream programs.  It may resemble
            the GNU Radio Companion to some extent.

quickstream APIs (application programming interfaces) are separated into the
the two classes of quickstream users:

  - *quickstream/app.h* for users to compile quickstream programs

  - *quickstream/block.h* for users to compile quickstream module blocks


The quickstream starting point is this web page at
https://github.com/lanceman2/quickstream.git, should it move we wish to
keep that URL current.

We keep the current Doxygen generated manual in html at
https://lanceman2.github.io/quickstream.doc/html/index.html


## Quick start

Install the [required core prerequisite packages](#required-core-prerequisite-packages),
and then run
```console
make
```
in the top source directory.   Then run
```console
make test
```

You do not have to install this package to use all of it's functionality.
See more details on building and installing below.


## Required, core, prerequisite packages

Building and installing quickstream requires the following debian package
prerequisites:

```shell
build-essential
```

## Optional prerequisite packages

You can install the following debian package prerequisites, but they
will are not required to build the core of quickstream:


```shell
apt install
 libudev-dev\
 libfftw3-dev\
 graphviz\
 imagemagick\
 doxygen\
 librtlsdr-dev\
 libasound2-dev\
 python3-dev\
 libssl-dev\
 libuhd-dev
```

If you do want to generate documentation you will need the three packages:
graphviz, imagemagick, and doxygen.   If you install any of these we
recommend installing all three, otherwise some of the make/build scripts
may fail.

If you do not currently have a particular application that you need to
make and/or you are not in the final optimizing of your application, we
recommend just installing all of these packages.  Then you will not have
as much unexpected heart ache, at the cost of installing some packages,
most of which you likely already have if you've got to this point in
life.


## Building and Installing with GNU Autotools

quickstream can (or will) be built and installed using a GNU Autotools
software build system.

If you got the source from a repository run

```console
./bootstrap
```

to add the GNU autotools build files to the source.

Or if you got this source code from a software tarball release change
directory to the top source directory and do not run bootstrap.

Then from the top source directory run

```console
./configure --prefix=/usr/local/PLACE_TO_INSTALL
```

where you replace */usr/local/PLACE_TO_INSTALL* with a directory where you
will like to have the *quickstream* software package installed.  Then run

```console
make
```

to build the code.  Then run

```console
make install
```

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

```console
./RepoClean
```

then make sure that the files that remain are what you want, if not you
may need to edit *RepoClean*.  After you're confident that is doing what
you want run:

```console
./bootstrap
```

which will download/generate many files needed for a tarball release.
Then run:

```console
./configure
```

which the is famous GNU autotools generated *configure* script for this
package.  Then run:

```console
make dist
```

which should generate the tarball file *quickstream-VERSION.tar.gz* and
other compression format versions of tarball files.


## Building and Installing with quickbuild

quickbuild is the quickstream developers build system.  Without some care
the quickbuild build system and the GNU autotools build system will
conflict with each other.  So why do we have two build systems?:

GNU autotools does not allow you to load dynamic shared object (DSO)
plugin modules without installing them first, whereby making a much longer
compile and test cycle.  The same, more-so, can be said of CMake.  As a
code developer we just want to quickly compile source and run it from the
source directory, without installing anything.  Imposing this requirement
on this project has the added benefit of forcing the source code to have
the same directory structure as the installed files, whereby making it
very easy to understand the file structure of the source code.

The use of quickbuild is for quickstream developers, not users.
quickbuild is just some "GNU make" make files and some bash scripts.  We
added these make files named "Makefile" and other files so that we could
develop, compile and run programs without being forced to install files.
We use "makefile" as the make file for GNU autotools generated make files,
which overrides "Makefile", at least for GNU make.


### quickbuild - quick and easy

Run:

```console
make
```

```console
make install PREFIX=MY_PREFIX
```

where MY_PREFIX is the top installation directory.  That's it,
if you have the needed dependences it should be installed.


### quickbuild with more options

If you got the quickstream source from a repository, or a tarball,
change directory to the top source directory and then run:

```console
./quickbuild
```

which will download the file *quickbuild.make* and generate *config.make*.
Now edit *config.make* to your liking.  Then run:

```console
make
```

to generate (compile) files in the source directory.  When building with
quickbuild, you can run programs from the source directories without
installing.  After running *make*, at your option, run:

```console
make install
```

to install files into the prefix (PREFIX) directory you defined.

Note the *PREFIX* will only be used in the *make install* and is not
required to be set until *make install* runs, so alternatively you can
skip making the *config.make* file and in place of running *make install*
you can run *make install PREFIX=MY_PREFIX*.

If you wish to remove all files that are generated from the build scripts
in this package, leaving you with just files that are from the repository
and your added costume files, you can run *./RepoClean*.  Do not run
*./RepoClean* if you need a clean tarball form of the package source,
use *make distclean* for that, or just keep the tarball around for that.

quickbuild does not does not have much in the line of options parsing,
which is fine for quickstream developers, but not so good for users;
hence GNU autotools is the preferred build/install method.  The optional
dependences that for not found when building with quickbuild will cause it
to exclude building what it can't automatically, but the GNU autotools
method adds configuration options.


## quickstream is Generic

Use it to process video, audio, radio signals, or any data flow that
requires fast repetitive data transfer between filter modules.


## quickstream is Fast

The objective is that quickstream flow graph should process data faster
than any other streaming API (application programming interface).
Only benchmarks can show this to be true.


## No connection types

The quickstream use case is generic, in the way it does not care what kind
of data is being transferred between filter stages, in the same way UNIX
pipes don't care what kind of data flows through them.  The types of data
that flows is up to you.  The typing of data flowing between particular
filters is delegated to a higher protocol layer above quickstream.
quickstream provides generic management for the connecting and running of
filter streams.  quickstream does not consider ports and channels between
filters to be typed.


## Restricting filters modules leads to user control and runtime optimization

quickstream restricts filter interfaces so that it may provide an
abstraction layer so that it may vary how it distributes threads and
filter modules at runtime, unlike all other high performance streaming
toolkits.  The worker threads in quickstream are not assigned to a given
filter module and migrate to filters modules that are in need of workers.
In this way threads do a lot less waiting for work, whereby eliminating
the need for threads that do a lot of sleeping, whereby eliminating lots
of system calls, and also eliminating inter thread contention, at runtime.
Put another way, quickstream will run less threads and keep the threads
busier by letting them migrate among the filter modules.


## Pass-through buffers

quickstream provides the buffering between module filters in the flow.
The filters may copy the input buffer and then send different data to it's
outputs, or if the output buffers are or the same size as the input
buffers, it may modify an input buffer and send it through to an output
without copying any data whereby eliminating an expensive copy operation.


## Description

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
to use to do a particular task, the choose should be obvious (at this
level).

The intent is to construct a flow stream between filters.  The filters do
not necessarily concern themselves with their neighboring filters; the
filters just read input from input ports and write output to output ports,
not necessarily knowing what is writing to them or what is reading from
them; at least that is the mode of operation at this protocol (quickstream
API) level.  The user may add more structure to that if they need to.
It's like the other UNIX abstractions like sockets, file streams, and
pipes, in that the type of data is of no concern in this quickstream
API.

The default implementation a "quickstream" program runs as a single
process and an on the fly adjustable number of worker threads.  The user
may restrict the number of worker threads at or below a user specified
maximum, which may be zero.  Having zero worker threads means that the
main thread will run the stream flow.  Having one worker thread means that
the one worker thread, only, will run the stream flow; but in that case
the main thread will be available for other tasks while the stream is
flowing/running.


## Terminology

To a limited extend we follow de facto standard terms from [GNU
radio](https://www.gnuradio.org/).  We avoided using GNU radio terminology
which we found to be confusing.  The scope of use for quickstream is
broader than GNU radio and so some GNU radio terminology was deemed not
generic enough.

### Stream

The directed graph that data flows in.   Nodes in the graph are called filters.


#### Stream states

A high level view of the stream state diagram (not to be confused with a
stream flow directed graph) looks like so:

![image of stream simple state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/stateSimple.png)

which is what a high level user will see.  It's like a very simple video
player with the video already selected.  A high level user will not see
the transition though the *pause* on the way to exit, but it is there.

If we wish to see more programming detail the stream state diagram can be
expanded to this:

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
  called, and stream filter connections are figured out.  Filter modules
  can also have their *destroy* functions called and they can be unloaded.
  Reasoning: *construct* functions may be required to be called so that
  constraints that only known by the filter are shown to the user, so that
  the user can construct the stream.  The alternative would be to have
  particular filter connection constrains exposed separately from outside
  the module dynamic share object (DSO), which would make filter module
  management a much larger thing, like most other stream toolkits.  We
  keep it simple at the quickstream programming level.
- **start**: the filters start functions are called. Filters will see how
  many input and output channels they will have just before they start
  running.  There is only one thread running.  No data is flowing in the
  stream when it is in the start state.
- **flow**: the filters feed each other and the number of threads running
  depends on the thread partitioning scheme that is being used.  A long
  running program that keeps busy will spend most of its time in the
  *flow* state.
- **flush**: stream sources are no longer being feed, and all other
  non-source filters are running until all input the data drys up.
- **stop**: the filters stop functions are being called. There is only one
  thread running.  No data is flowing in the stream when it is in the
  stop state.
- **destroy** in reverse load order, all remaining filter modules
  *destroy* functions are called, if they exist, and then they are
  unloaded.
- **return**: no process is running.

The stream may cycle through the flow state loop any number of times,
depending on your use, or it may not go through the flow state at all,
and go from pause to exit; which is a useful case for debugging.


### Block

A block is the fundamental building block of quickstream which as a
compiled code that has a limited number of "quickstream standard" callback
functions in it.  The smallest number of standard callback functions is
zero.  In quickstream all blocks are treated same.  There is no top block.


### Controller

or controller-block.  A block module that does not have stream data input
or stream data output is called a controller-block or controller for
short.  The controller's work function, if it exists, will only be
triggered by the state of OS (operating system) file descriptors or from
other callbacks in this block which queue a work call.


### Filter

or filter-block.  A block module that reads input and/or writes outputs in
the stream is called a filter-block of filter for short.  The number of
input channels and the number of output channels to the filter may be any
integer.  Filters do not know if they are running as a single thread by
themselves or they are sharing their thread execution with other filters.
From the quickstream user, the filters just provide executable object code
in the form of C/C++ functions.  The filters work function optionally
reads input and writes output.  At the quickstream module writer level the
filters don't know what filters they are reading and writing to.  In
addition to reading and writing input and output, filters can also preform
all the same functions as controller-blocks, though one may find that
separating such functionality into separate controller-blocks less
convoluted.


### Parameter

A single value or small data structure that can be shared between blocks
via "get" or "set".


### Monitor

Or monitor-block: That which monitors a other blocks, without changing how
any of them process inputs, write outputs, or set other blocks parameters.
A monitor may create and own a parameter that is derived by parameters
owned by other blocks, or general internal state of quickstream.


### Channel

Channel is a connection to a filter from another filter.  For a given
filter running in a stream at a particular time, there are input channels
and output channels.  For that filter running in a given cycle the input
and output channels are numbered from 0 to N-1, where N is the number of
input or output channels.  The filters may decide how many input and
output channels they will work with.


### Shared buffers and Port

Channels connect any number of output ports to one input port.  There can
only be one writing filter at a given channel level.  If there is a
"pass-though" channel, than there can a trailing "pass-through" writer at
each "pass-though" level.  The "pass-through" writer writes behind all the
reading ports in the level above.


### Source

A filter module with no inputs is a source filter.  It may get "input"
from something other than the stream, like a file, or a socket, but those
kinds of inputs are external from the quickstream, that is they do not get
any input from the quickstream buffering system.  So in a more global
sense they may not be sources with data input, but with respect to
quickstream they are sources which have no data input.


### Sink

A filter module with no outputs is a sink filter.  It may write "output"
to something other than the stream, like a file, a socket, or display
device, but those kinds of outputs are external from the quickstream, that
is they do not read from quickstream buffering system using the
quickstream API.  In a more global sense that may not be sinks and they
may produce output, but with respect to quickstream they are sinks and
they produce no quickstream output channels.


## Interfaces

quickstream has two APIs (application programming interfaces) and some
utility programs.  Both APIs in use the libquickstream library.  The main
parts are:

- a filter API **filter.h**: which is used to build a quickstream filter
  dynamic shared object filter modules may provide predeclared functions.
- a stream program API **app.h**: which is used to build programs that run
  a quickstream with said filters modules.
- the program **quickstream**: which uses *app.h* and libquickstream
  library to run a quickstream flow graph with said filters.

quickstream can load the same filter module more than once.  Each loaded
filter module will run in a different shared object library space.


## OS (operating system) Ports

Debian 9 and Ubuntu 18.04


## Similar Software Projects

Most other stream flow like software projects have a much more particular
scope of usage than quickstream.  These software packages present a lot of
the same ideas.  We study them and learn.

- **GNUradio** https://www.gnuradio.org/
- **gstreamer** https://gstreamer.freedesktop.org/
- **RedHawk** https://geontech.com/redhawk-sdr/
- **csdr** https://github.com/simonyiszk/csdr We like straight forward way
  csdr uses UNIX pipe-line stream to make its flow stream.


### What quickstream intends to do better

Summary: Run faster with less system resources.  Be simple.

In principle it should be able to run fast.  We have no controlling task
manager thread that synchronizes the running threads.  We let the
operating system do its' thing, and try not to interfere with a task
managing code.  If any task management is needed we let it happen in a
higher layer.  All filters keep processing input until they are blocked by
a slower down-stream filter (clogged), or they are waiting for input from
an up-stream filter (throttled).  In general the clogging and throttling
of the stream flows is determined by the current inter-filter buffering
parameters.

"Simple" filters can share the same thread, and whereby use less system
resources, and thereby run the stream faster by avoiding thread context
switches using less time and memory than running filters in separate
threads (or processes).  When a filter gets more complex we can let the
filter run in it's own thread (or process).

It's simple.  There is no learning curve.  There's just less there.  There
is only one interface for a given quickstream primitive functionality.  No
guessing which class to inherit.  No built-in inter-filter data typing.
We provide just a modular streaming paradigm without a particular end
application use case.  The idea of inter-module data types is not
introduced, that would limit it's use, and can be considered at a higher
software interface layer.  So we keep inter-module data typing outside
the scope of quickstream.

In the future benchmarking will tell.  TODO: Add links here...


## A Typical quickstream Flow Graph

![simple stream state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/simpleFlow.png)

## A detailed quickstream Flow Graph

![complex stream state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/complexFlow.png)


## Other graphviz dot quickstream development images

![job flow and related data structures](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/jobFlow.png)



## Developer notes

- quickstream code is written in fairly simple C with very few dependences.
  Currently runtime libraries linked are -lpthread -ldl and -lrt.
- The API (application programming interface) user sees data structures as
  opaque.  They just know that they are pointers to data structures, and
  they do not see elements in the structures.  We don't use typedef, for
  that extra layer of indirection makes it harder to "scope the code".
- The files in the source directly follow the directory structure of the
  installed files.  Files in the source are located in the same relative
  path of files that they are most directly related to in the installed
  paths.  Consequently:
  - you don't need to wonder where source files are,
  - programs can run in the source directory after running "make" without
    installing them,
  - the running programs find files from a relative paths between them,
    the same way as in the installed files as with the files in the
    source,
  - we use the compilers relative linking options to link and run
    programs (libtool may not do this, but quickbuild does), and
  - you can move the whole encapsulated installation (top install
    directory) and everything runs the same.
    - there are not full path strings compiled in the libarary.
- Environment variables allow users to tell quickstream programs where to
  find users files that are not in the quickstream source code.
- The installation prefix directory is not used in the quickstream code,
  only relative paths are needed for running quickstream files to find
  themselves.  On a bad note: One needs to worry about package
  installation schemes that brake the encapsulation of the installed
  files.
- C++ (TODO: RUST) code can link with quickstream.
- The public interfaces are object oriented in a C programming sense.
- The private code is slightly more integrated than the public interfaces
  may suggest.
- We wish to keep this C code small and manageable.
  - Run: *./dev_tests/linesOfCCode.bash* to see the number of lines of
    code.
- Minimize the use of CPP macros, so that understanding the code is easy
  and not a CPP macro nightmare like many C and C++ APIs.  Every
  programmer knows that excessive use of CPP macros leads to code that is
  not easily understandable in the future after you haven't looked at it
  in a while.
- We tend to make self documenting interfaces with straight forward variable
  arguments that are provide quick understanding, like for example we prefer
  functions like **read(void *buffer, size_t len)** to
  **InputObject->val** and **InputObject->len**.  The read form is self
  documenting where as an input object (or struct) requires that we force
  the user to lookup methods to get to the wrapped buffer data.  We don't
  make abstractions and wrappers unless the added benefit for the user
  outweighs the cost of adding new uncommon abstractions.
- The standard C library is a thing we use.  The GNU/Linux and BSD (OSX)
  operating systems are built on it.  For the most part C++ has not
  replaced it, but has just wrapped it.
- The linux kernel and the standard C library may be old but they are
  still king, and used by all programs whither people know it or not.
- If you wish to make a tarball release use the GNU autotool building
  method and run *./configure* and *make dist*.
- There will be no downloading of files in the build step.  Downloading
  may happen in the bootstrap step.  In building from a tarball release
  there will be no downloads or bootstrapping.  Packages that do this
  make the job of distributing software very difficult, and are
  suggestive of poor quality code.
- Files in the source are located in the same relative path of files that
  they are most directly related to in the installed paths.
- All filter modules do not share the global variable space that came from
  the module.  Particular filter modules may be loaded more than once.
  Most module loading systems do not consider this case, but with simple
  filters there is a good chance you may want to load a filter more than
  once, with the same filter plugin in different positions in the stream.
  As a result of considering this case, we have the added feature that the
  filter module writer may choose to dynamically create objects, or make
  them statically.  In either case the data in the modules stays
  accessible only in that module.  Modules that wish to share variables
  with other modules may do so by using other non-filter DSOs (dynamic
  shared objects), basically any other library than the filter DSO.
- quickstream is not OOP (object oriented programming).  We only make
  objects when it is necessary.  Excessive OOP can lead to more complexity
  and bloat, which we want to avoid.
- Don't put stuff in quickstream that does not belong in quickstream.  You
  can always build higher level APIs on top of quickstream.
- If you don't like quickstream don't use it.


## Driving concerns and todo list

- Simplicity and performance.

- Is this quickstream idea really like a layer in a protocol stack that
  can be built upon?

    * Sometimes we need to care about the underlying workings that
      facilitate an abstraction layer because we have a need for
      performance.
      - It would appear that current software development trends frowns
        upon this idea.

- Bench marking with other streaming APIs; without which quickstream will
  die of obscurity.

- It looks like a kernel hack is not needed, or is it?  mmap(2) does what
  we need.  NPTL (Native POSIX Threads Library) pthreads rock!

- Add "super filter modules" that load groups of filter modules.

- https://www.valgrind.org/ ya that added to development tests.


## Things to do before a release

- Check that no extra (unneeded) symbols are exposed in libquickstream.
- Run all tests with compile DEBUG CPP flag set.
- Run all tests with compile DEBUG CPP flag unset.
- Run all tests without compiler flag -g.
- Make doxygen run without any warnings.
- Proof all docs including this file.
- Make the base build build in less than 1 minute (last check is about 10
  seconds).

