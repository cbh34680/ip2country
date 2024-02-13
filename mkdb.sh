#!/bin/bash

set -eu

: ${2?"Usage: $0 cidr.txt output-path"}

[ -f $1 ] || { echo $1: file not found; exit 1; }

tmpf=$(mktemp)
trap "rm -f $tmpf" EXIT

#
# from: COUNTRY NETWORK/MASK
# to:   COUNTRY MASK NETWORK[0] NETWORK[1] NETWORK[2] NETWORK[3]
#
# sort by NETWORK
#
< $1 tr '/' ' ' | tr '.' ' ' | sort -k2,2n -k3,3n -k4,4n -k5,5n \
| awk '{print $1,$6,$2,$3,$4,$5}' > $tmpf

#
# from text to binary
#
# format)
#    4byte network
#    4byte broadcast
#    1byte mask
#    1byte padding
#    2byte country
#
/usr/bin/python3 txt2bin.py -i $tmpf -o $2

exit 0

