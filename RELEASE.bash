#!/bin/bash
#
# In here are numbers for a particular release of this software.
#
# We also keep in here a bash function that replaces things like
# for example replace @PACKAGE_VERSION@ with 5.3.2



#
# Usage: ReplaceReleaseStrings [INFILE]
#
# Writes to stdout INFILE with "release" strings like
# @PACKAGE_VERSION@ replaced.  If INFILE is not present
# it prints all the release strings to stdout.
#
function ReplaceReleaseStrings() {

# declare bash associative array r
    declare -A r
# keep it local so it's polluting the namespace
    local r=(
##########################################################################
##########################################################################
##   BEGIN:    PACKAGE RELEASE STRINGS                                  ##
##########################################################################
##########################################################################

# We did not make these names up.  Most of these string names (keys) are
# standardized by GNU autotools.

# Do not add things in here that may be computed in the user configuration
# step; like things that a user that is building from source may set.
# Values in this file will not change due to user configuration.
#


# Change the value of these components of the PACKAGE_VERSION to make a
# new release by changing the major minor and edit numbers here:


        [PACKAGE_MAJOR]=0
        [PACKAGE_MINOR]=0
        [PACKAGE_EDIT]=1


# installed library version numbers
#
# library version number changes must follow API and ABI compatibility
# rules.  See comments in the bottom of configure.ac.in about library
# version numbers which are not necessarily related to package release
# versions.
#

# For the library libqsapp.so the stream app library

        [LIBQSAPP_CURRENT]=0
        [LIBQSAPP_REVISION]=0
        [LIBQSAPP_AGE]=0


# For the library libqsfilter for linking with filter modules

        [LIBQSFILTER_CURRENT]=0
        [LIBQSFILTER_REVISION]=0
        [LIBQSFILTER_AGE]=0



        [PACKAGE_BUGREPORT]="https://github.com/lanceman2/quickstream"

        [PACKAGE_URL]="https://github.com/lanceman2/quickstream"


# The package name is used to make tarball filenames.  Not likely
# needed to be changed.  This is the full name of the package.
#
        [PACKAGE_NAME]=quickstream


##########################################################################
##########################################################################
#   END:     PACKAGE RELEASE STRINGS                                    ##
##########################################################################
##########################################################################
    )

    # compose some release strings.  All standardized by GNU autotools:
    r+=([PACKAGE_VERSION]="${r[PACKAGE_MAJOR]}.${r[PACKAGE_MINOR]}.${r[PACKAGE_EDIT]}")
    r+=([PACKAGE]="${r[PACKAGE_NAME]}")
    r+=([PACKAGE_TARNAME]="${r[PACKAGE_NAME]}")
     

    local key

    if [ -z "$1" ] ; then
        for key in "${!r[@]}"; do
            echo "$key=\"${r[$key]}\""
        done
        return
    fi

    local replace=

    for key in "${!r[@]}"; do

        #
        # replace . with \. in the string so sed will not misinterpret it
        #
        rstr="${r[$key]//[\.]/\\.}"
        #
        # now replace the / with \/
        #
        rstr="${rstr//[\/]/\\\/}"

        replace="${replace} -e s/@${key}@/${rstr}/g"
    done

    echo "# This is a generated file"
    sed$replace "$1"
}


ReplaceReleaseStrings "$1"

