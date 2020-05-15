#!/bin/bash

scc_ver=6.80
scc_tag="release/${scc_ver}"
scc_package=scc-snapshots-${scc_ver}
scc_srcdir=${scc_package}_src
sha512sum=


if [ -n "$1" ] && [ "$1" != "-w" ] ; then
    cat << EOF

  Usage: $(basename $0) [-w]|[-h|--help]

    Print the number of lines of C code in this package, both counting
    comments and excluding comments.

    This will automatically download and build scc-snapshots (by Jonathan
    Leffler) which is a C and C++ code that removes comments from C and
    C++ code.


   --------------------------- OPTIONS ------------------------------------

        -h|--help  print this help and exit 1.

        -w         with comments and blank lines.  Otherwise it prints the
                   number of lines of C code excluding comments and blank
                   lines.

EOF

    exit 1
fi

case ${scc_tag} in

    release/6.80)
        sha512sum=648d0a0a95fd41b29a9fa268e9ed0\
e63a421519c6a015824404a1af5b2658ba33282548fc684\
f09b83c5f485fa380981754876408f1e3171aaa20f7e73645026
        ;;
esac


set -eo pipefail

dir="$(dirname $0)"

cd "$dir"
dir="$PWD"

stripcmt="$dir/$scc_srcdir/scc"


if [ ! -e "$stripcmt" ] && [ -z "$1" ] ; then

    # Download and build the Strip C Comments program scc
    # by Jonathan Leffler

    url="https://github.com/jleffler/scc-snapshots/tarball/$scc_tag"
    wget $url -O ${scc_package}.tgz

    if [ -n "$sha512sum" ] ; then
        echo "$sha512sum  ${scc_package}.tgz" |\
          sha512sum -c
    else
        sha512sum ${scc_package}.tgz
    fi

    rm -rf $scc_srcdir
    mkdir $scc_srcdir
    cd $scc_srcdir
    tar --strip-components=1 -xzf ../${scc_package}.tgz
    make
    cd ..
fi



cd ..

echo -e "\n\nTotal lines of C/C++ code *.c *.cpp and *.h *.hpp files in $PWD"

function spewCode() {

    for i in $(find . -regextype egrep -regex ".*/.*\.[c|h](pp|)$" | grep -v scc-snapshots-) ; do
        cat $i
    done
}

if [ -z "$1" ] ; then
    echo -e "without comments or blank lines.\n"
    spewCode |\
        $stripcmt |\
        sed -e 's/^\s*\/\/.*//g' -e '/^\s*$/d' |\
        wc -l
else
    echo -e "with comments and blank lines.\n"
    spewCode | wc -l
fi
