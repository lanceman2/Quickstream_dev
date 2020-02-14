#!/bin/bash

set -eo pipefail

cd "$(dirname ${BASH_SOURCE[0]})"

../../../bin/quickstream\
 -f stdin [ --name input ]\
 -f tests/copy [ --name transWarpConduit ]\
 -f tests/copy [ --name malulator ]\
 -f stdout [ --name output ]\
 -c\
 -g
