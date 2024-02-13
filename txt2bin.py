import sys
import os
import argparse
import struct

'''

< cidr.txt tr '/' ' ' \
| tr '.' ' ' \
| sort -k2,2n -k3,3n -k4,4n -k5,5n \
| awk '{print $1,$6,$2,$3,$4,$5}' \
> cidr-sorted.txt

python3 txt2bin.py -i cidr-sorted.txt -o cidr.bin

xxd -c 12 cidr.bin

'''


def txt2bin(args):
    MSB = 0x8000_0000
    ALLON = 0xFFFF_FFFF

    nwbits = [0] * 33
    bcbits = [0] * 33
    v = 0

    for i in range(33):
        nwbits[i] = v
        bcbits[i] = v ^ ALLON
        v = (v >> 1) | MSB

        #print(f'[{i}] {hex(nwbits[i])} {hex(bcbits[i])}')

    #print(nwbits)
    #print(bcbits)

    OUTNUM = 30
    #OUTIDX = 119760
    OUTIDX = None

    with open(args.input) as fin, open(args.output, 'wb') as fout:
        wcnt = 0

        for n, line in enumerate(fin):
            #if n == 119785:
            #    continue

            if not OUTIDX is None:
                if n < OUTIDX:
                    continue
                elif n > (OUTIDX + OUTNUM - 1):
                    break

            ccd, *nums, = line.strip().split()
            masknum, *nwaddr = map(int, nums)

            nw = nwaddr[0] << 24 | nwaddr[1] << 16 | nwaddr[2] << 8 | nwaddr[3]
            bc = nw | bcbits[masknum]
            bcaddr = [(bc & 0xff00_0000) >> 24, (bc & 0xff_0000) >> 16, (bc & 0xff00) >> 8, bc & 0xff]

            nwstr = '.'.join(map(str, nwaddr))
            bcstr = '.'.join(map(str, bcaddr))

            #print(f'{wcnt:5} {n} [{ccd}] [{nwstr}] [{masknum}] [{bcstr}]')

            ccd2 = ccd[0:2].encode('ascii')
            fout.write(struct.pack('!LLBx2s', nw, bc, masknum, ccd2))
            wcnt += 1

            lastn = n

        print(f'{wcnt} records writed', file=sys.stderr)

    pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', dest='input', required=True)
    parser.add_argument('-o', dest='output', required=True)
    args = parser.parse_args()

    txt2bin(args)
    

main()
