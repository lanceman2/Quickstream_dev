#!/bin/bash

# This script downloads (tarball) and installs valgrind

set -exo pipefail

cd "$(dirname ${BASH_SOURCE[0]})"

make_opts="-j $(nproc)"

tag=3.15.0

tarname=valgrind-$tag

tarfile=$tarname.tar.bz2

url=https://sourceware.org/pub/valgrind/$tarfile

prefix=/usr/local/encap/$tarname


sha512sum=

case "$tag" in
    3.15.0)
        sha512sum=5695d1355226fb63b0c80809ed43bb077\
b6eed4d427792d9d7ed944c38b557a84fe3c783517b921e32f1\
61228e10e4625bea0550faa4685872bb4454450cfa7f
        ;;
esac


if [ ! -e "$tarfile" ] ; then
    wget $url -O $tarfile
fi

if [ -n "$sha512sum" ] ; then
    echo "$sha512sum  $tarfile" | sha512sum -c -
else
    set +x
    sha512sum $tarfile
    set -x
fi

try=0
while [ -d "${tarname}-try-$try" ] ; do
    let try=$try+1
done
builddir="${tarname}-try-$try"

mkdir "$builddir"
cd "$builddir"
tar --strip-components=1 -xjf ../$tarfile

./configure --prefix=$prefix
make $make_opts
make install

set +x
echo "SUCCESSFULLY installed $tarname in $prefix"
