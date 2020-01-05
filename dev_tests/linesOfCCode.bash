#!/bin/bash

scc_ver=6.80
scc_tag="release/${scc_ver}"
package=scc-snapshots-${scc_ver}

set -eo pipefail

dir="$(dirname $0)"

cd "$dir"
dir="$PWD"

stripcmt="$dir/$package/scc"



if [ ! -e "$stripcmt" ] ; then

    # Download and build the Strip C Comments program scc
    # by Jonathan Leffler

    url="https://github.com/jleffler/scc-snapshots/tarball/$scc_tag"
    wget $url -O ${package}.tgz
    rm -rf $package
    mkdir $package
    cd $package
    tar --strip-components=1 -xzf ../${package}.tgz
    make
    cd ..
fi



cd ..

echo -e "\n\nTotal lines of C code *.c and *.h files in $PWD"
echo -e "without comments or blank lines.\n"

function spewCode() {

    for i in $(find . -regextype sed -regex ".*/.*\.[c|h]$" | grep -v scc-snapshots-) ; do
        cat $i
    done
}


spewCode |\
    $stripcmt |\
    sed -e 's/^\s*\/\/.*//g' -e '/^\s*$/d' |\
    wc -l

