
cc -Wall -O2 -o ip2country ip2country.c && strip ./ip2country  
./ip2country cidr.bin 8.8.8.8

