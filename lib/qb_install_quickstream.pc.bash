#!/bin/bash

# This is only used when building quickstream with quickbuild.  If you
# need more file path like options use the GNU autotools to build
# quickstream.

set -e

cd $(dirname ${BASH_SOURCE[0]})

if [ -z "$1" ] ; then
    echo "Usage: $0 PREFIX"
    exit 1
fi

PREFIX="$1"

declare -A r=(

[prefix]="$PREFIX"
[exec_prefix]="$PREFIX"
[libdir]="$PREFIX/lib"
[includedir]="$PREFIX/include"
[datadir]="$PREFIX/share"
)


tmpfile=qb_build/quickstream.pc.tmp
../RELEASE.bash pkgconfig/quickstream.pc.in > $tmpfile

for key in "${!r[@]}"; do

    #
    # replace . with \. in the string so sed will not misinterpret it
    #
    rstr="${r[$key]//[\.]/\\.}"
    #
    # now replace the / with \/ for sed
    #
    rstr="${rstr//[\/]/\\\/}"
    replace="${replace} -e s/@${key}@/${rstr}/g"
done

mkdir -p "${r[libdir]}/pkgconfig/"
outfile="${r[libdir]}/pkgconfig/quickstream.pc"

set -x
echo "# This is a generated file" > "$outfile"
sed$replace $tmpfile >> "$outfile"
rm $tmpfile
