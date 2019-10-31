
This file just used for quickstream design discussion in a free format,
given that quickstream is not in an alpha (usable) state yet.




## Add more automatic buffer sizing.

  These example cases will not happen but in a couple stream flow cycles.

  For example:
  
  1. a qsOutput() call with a
     length that would overflow the buffer, will cause the buffer to
     increase its' size.
  2. a qsGetBuffer() call with a length that would overflow the buffer,
     will cause the buffer to increase its' size.


## filter options and parameters at *construct()* and *start()*

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



