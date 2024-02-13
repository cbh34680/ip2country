#!/usr/bin/python3

import os
import sys
import ipaddress

def main():
    for line in ( x.strip() for x in sys.stdin.readlines() ):
        if '/' in line:
            net = ipaddress.ip_network(line)
            if not net.is_private:
                for x in ipaddress.ip_network(line).hosts():
                    print(x)
        else:
            addr = ipaddress.ip_address(line)
            if not addr.is_private:
                print(line)

if __name__ == '__main__':
  main()

