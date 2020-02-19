#!/bin/bash
#
# In here are numbers for a particular release of this software.
#
# We also keep in here a bash function that replaces things like
# for example replace @PACKAGE_VERSION@ with 5.3.2

##########################################################################
#         Making a tarball release
#
#  A tarball release has additional generated files when compared to the
#  files checked into a git repository.  Some of those additional files
#  are downloaded from the web, in addition to, local file generation.
#
#  If we do it correctly the tarball that we generate from a given set of
#  git repository checkouts will be unique.
#
#  We do not want to get caught in the trap where we require a specific
#  kind of software repository to develop the release code.  So, for
#  example, we can never add 'git' commands to scripts in this source
#  code, especially for making release numbers.

# There is a problem with this file.  Some of the strings that we define
# here, in this file, are also in README.md, but README.md cannot be a
# generated file because it must be part of the git repository.  Ya, 'git
# describe' is handy but we don't want building our software to dependent
# on 'git' being installed.



#
# Usage: ReplaceReleaseStrings [INFILE [COMMENT_PREFIX]]
#
# Writes to stdout INFILE with "release" strings like @PACKAGE_VERSION@
# replaced.  If no argument options are present this script prints all the
# release strings to stdout.
#
function ReplaceReleaseStrings() {

# declare bash associative array r
    declare -A r
# keep it local so it's not polluting the namespace
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

# For the library libquickstream.so the stream app library

        [LIBQS_CURRENT]=0
        [LIBQS_REVISION]=0
        [LIBQS_AGE]=0



        [PACKAGE_BUGREPORT]="https://github.com/lanceman2/quickstream"

        [PACKAGE_URL]="https://github.com/lanceman2/quickstream"

        [WEB_DOCS_URL]="https://github.com:lanceman2/quickstream.doc"


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

    # compose some release strings from the above strings.
    # Most are standardized by GNU autotools:

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

    echo "${2}# This file is generated for a given software release"
    sed$replace "$1"
}


ReplaceReleaseStrings $*
