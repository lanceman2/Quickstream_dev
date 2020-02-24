# We called this file configure.ac.rl and not configure.ac.in so that we
# did not get built in make rules for .in file missing with it.


# AC_INIT( PACKAGE_NAME, PACKAGE_VERSION, BUGREPORT, TARNAME , URL)
AC_INIT([@PACKAGE_NAME@],
        [@PACKAGE_VERSION@],
        [@PACKAGE_BUGREPORT@],
        [@PACKAGE_TARNAME@],
        [@PACKAGE_URL@])



# See LIBRARY VERSION notes at the bottom of this file.
#

LIBQS_CURRENT=@LIBQS_CURRENT@
LIBQS_REVISION=@LIBQS_REVISION@
LIBQS_AGE=@LIBQS_AGE@


#AM_INIT_AUTOMAKE(-Wall -Werror dist-bzip2 no-dist-gzip subdir-objects)
AM_INIT_AUTOMAKE(-Wall dist-bzip2 dist-xz subdir-objects)
AC_CONFIG_MACRO_DIRS([m4])
AM_PROG_AR
AC_PROG_LIBTOOL
LT_INIT([disable-static])
AC_PROG_CC
AC_PROG_CXX

LIBQS_VERSION="${LIBQS_CURRENT}:${LIBQS_REVISION}:${LIBQS_AGE}"


AC_SUBST(LIBQS_VERSION)
AC_SUBST(LIBQS_CURRENT)
AC_SUBST(LIBQS_REVISION)
AC_SUBST(LIBQS_AGE)

AC_DEFINE_UNQUOTED(LIBQS_VERSION, "$LIBQS_VERSION",
                   [quickstream library version number])


# We uncomment this for testing.
#AC_CONFIG_HEADERS([config.h])

################################################################
#                 --enable-debug
################################################################

AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug],
    [compile more debugging code into quickstream and libquickstream\
 (default is no debugging code).  You do not want this unless\
 you are debugging or developing quicksteam.  Adding this option may make\
 use about 10% more CPU usage.  This is unrelated to the compiler\
 options used.]),
 [enable_debug="$enableval"],
 [enable_debug=no])

case "$enable_debug" in
    y* | Y* )
    enable_debug="-DDEBUG "
    debug="yes"
    ;;
    * ) # default
    enable_debug=""
    debug="no"
    ;;
esac


AC_ARG_ENABLE([spew-level],
    AS_HELP_STRING([--enable-spew-level=error|warn|notice|info|debug|none],
    [compile more spewing code into quickstream and libquickstream\
 (default is no spewing code).  You do not want this unless\
 you are debugging or developing quickstream.  Adding this option may make\
 use about 11% more CPU usage.  This is unrelated to the compiler\
 options used.]),
 [enable_spew_level="$enableval"],
 [enable_spew_level=zdefault])

case "$enable_spew_level" in
    e*|E* )
    enable_spew_level="-DSPEW_LEVEL_ERROR"
    spew_level=none
    ;;
    w*|W* )
    enable_spew_level="-DSPEW_LEVEL_WARN"
    spew_level=warn
    ;;
    not*|NOT* )
    enable_spew_level="-DSPEW_LEVEL_NOTICE"
    spew_level=notice
    ;;
    i*|I* )
    enable_spew_level="-DSPEW_LEVEL_INFO"
    spew_level=info
    ;;
    d*|D* )
    enable_spew_level="-DSPEW_LEVEL_DEBUG"
    spew_level=debug
    ;;
    * ) # default
    enable_spew_level="-DSPEW_LEVEL_NONE"
    spew_level=none
    ;;
esac



# AC_SUBST() go into Makefile.am files and other .in files
# AC_DEFINE*() go into config.h

AM_CPPFLAGS="$enable_debug$enable_spew_level"

AC_SUBST([AM_CPPFLAGS]) 


AC_CONFIG_FILES(
 makefile
 include/quickstream/makefile
 include/quickstream/app.h
 lib/makefile
 lib/pkgconfig/quickstream.pc
 bin/makefile
 lib/quickstream/plugins/filters/makefile
 lib/quickstream/plugins/filters/tests/makefile
 share/doc/quickstream/prefix.make
 share/doc/quickstream/makefile
 share/bash-completion/completions/makefile
 lib/quickstream/misc/makefile
)

AC_OUTPUT

# you can add spew here

AC_MSG_RESULT([
  ------------------------------------------------------------
    done configuring: $PACKAGE_NAME  version $VERSION
  ------------------------------------------------------------

              extra debug code (--enable-debug): $debug
                 extra spew code (--spew-level): $spew_level

                   C Compiler (CC): $CC
   C Preprocesser Flags (CPPFLAGS): $CPPFLAGS
         C Compiler Flags (CFLAGS): $CFLAGS
          C Linker Flags (LDFLAGS): $LDFLAGS

    Installation prefix (--prefix): $prefix
                            srcdir: $srcdir
])


######################################################################
#                     LIB_VERSION notes
######################################################################

#  Updating library version interface information
#  (from libtool version 1.4.3 info page)

# GNU LibTool library interface numbers
#
# CURRENT
#     The most recent interface number that this library implements.
#
# REVISION
#     The implementation number of the CURRENT interface.
#
# AGE
#     The difference between the newest and oldest interfaces that
#     this library implements.  In other words, the library implements
#     all the interface numbers in the range from number `CURRENT -
#     AGE' to `CURRENT'.
#

#
#  1. Start with version information of `0:0:0' for each libtool
#     library.
#
#  2. Update the version information only immediately before a public
#     release of your software.  More frequent updates are
#     unnecessary, and only guarantee that the current interface
#     number gets larger faster.  We do it just after a release.
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public
#     release, then set AGE to 0.
#
