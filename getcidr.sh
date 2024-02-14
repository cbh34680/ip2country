#!/bin/bash

set -eux

: ${1?"Usage: $0 output-path"}
outf=$1

tmpgzf=$(mktemp --suffix='.gz')
trap "rm -f $tmpgzf" EXIT

#
# IPv4 address allocation list by country around the world
#
if [ -f $outf ]
then
  statopt='-c %Y'
  [ $(uname) = 'Darwin' ] && statopt='-f %m'
  cidrmdt=$(stat $statopt $outf)

  [ $(( $(date +'%s') - $cidrmdt )) -gt 604800 ] && rm $outf
fi

if [ ! -f $outf ]
then
  curl -sSL -o $tmpgzf http://nami.jp/ipv4bycc/cidr.txt.gz
  gzip -dc $tmpgzf > $outf
fi

test $(< $outf wc -l) -gt 100000

exit 0

