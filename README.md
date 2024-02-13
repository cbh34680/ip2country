
bash mkdb.sh cidr.txt country-db.bin  

cc -Wall -O2 -o ip2country ip2country.c && /usr/bin/strip ./ip2country  
./ip2country country-db.bin 8.8.8.8  

