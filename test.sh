#!/bin/bash

unalias -a
cd $(dirname $(readlink -f "${BASH_SOURCE:-$0}"))

# readlink_f(){ perl -MCwd -e 'print Cwd::abs_path shift' "$1";}

set -eux

#
# IPv4 address allocation list by country around the world
#
if [ -f cidr.txt ]
then
  if [ $(( $(date +'%s') - $(stat -c '%Y' cidr.txt) )) -gt 604800 ]
  then
    rm cidr.txt
  fi
fi

if [ ! -f cidr.txt ]
then
  curl -sSLO http://nami.jp/ipv4bycc/cidr.txt.gz
  gunzip -f cidr.txt.gz
fi

#
# from: COUNTRY NETWORK/MASK
# to:   COUNTRY MASK NETWORK[0] NETWORK[1] NETWORK[2] NETWORK[3]
#
# sort by NETWORK
#
< cidr.txt tr '/' ' ' | tr '.' ' ' | sort -k2,2n -k3,3n -k4,4n -k5,5n \
| awk '{print $1,$6,$2,$3,$4,$5}' > cidr-sorted.txt

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
python3 txt2bin.py -i cidr-sorted.txt -o cidr.bin

#
# get data (sample)
#

# [github]
#curl -sS https://api.github.com/meta | jq -r .hooks[] | fgrep -v ':' | python3 expand-net.py > sample.txt

# [google]
#curl -sS https://www.gstatic.com/ipranges/goog.json | jq -r '.prefixes[] | select(.ipv4Prefix != null).ipv4Prefix' | python3 expand-net.py > sample.txt

# [root nameserver]
(
  dig +short -t ns . | while read name
  do
    dig +short -t a $name
  done
) > sample.txt

#
# get country for each address 
#
cc -Wall -O2 -o ip2country ip2country.c
strip ip2country

set +x

while read addr
do
  code=$(./ip2country cidr.bin $addr 2>/dev/null)
  rc=$?

  echo "${addr} ${code} ${rc}"

done < sample.txt

exit 0

