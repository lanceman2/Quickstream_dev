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
and usability than it's previous generation.

The quickstream starting point is this web page at
https://github.com/lanceman2/quickstream.git, should it move we wish to
keep that URL current.


## Development Status

It's not functional yet.  Currently it's got most of directory structure
laid out, build system, and components figured out.  The thing of interest
now for you may be this README.md file.  Current development is on Debian
9 and Ubuntu 18.04 systems.


## Prerequisite packages

Building and installing quickstream requires the following debian package
prerequisites:

```shell
build-essential
graphviz
imagemagick
doxygen
```


## Building and Installing with GNU Autotools

quickstream can be built and installed using a GNU Autotools software
build system.

If you got the source from a repository run

```console
./bootstrap
```

to add the GNU autotools build files to the source.

  Or if you got this
source code from a software tarball release change directory to the
top source directory and do not run bootstrap.

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

which will generate many files needed for a tarball release.  Then run:

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


### quickbuild quick and easy

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
to use to do a particular task, the choose should be obvious.

The intent is to construct a flow stream of filters.  The filters do not
necessarily concern themselves with their neighboring filters; the filters
just read input from input ports and write output to output ports, not
necessarily knowing what is writing to them or what is reading from them;
at least that is the mode of operation at this protocol (quickstream API)
level.  The user may add more structure to that if they need to.  It's
like the other UNIX abstractions like sockets, file streams, and pipes, in
that the type of data is of no concern in this quickstream APIs.


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


### Filter

A module reads input and writes outputs in the stream.  The number of
input channels and the number of output channels may be from zero to any
integer.  Filters do not know if they are running as a single thread by
themselves or they are sharing their thread execution with other
filters.  From the quickstream user, the filters just provide executable
object code in the form of C/C++ (maybe RUST) functions.  The filters just
read input and write output.  At the quickstream level the filters don't
know what filters they are reading and writing to.


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
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/quickstream_simple.png)

![complex stream state](
https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/quickstream_complex.png)


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
  make the job of distributing software very difficult, and suggest poor
  quality.
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

