
all: ip2country country-db.bin

ip2country: main.c fbsrch.c fbsrch.h
	cc -Wall -O2 -o ip2country main.c fbsrch.c
	/usr/bin/strip ip2country

country-db.bin: cidr.txt
	/bin/bash mkdb.sh cidr.txt country-db.bin

cidr.txt:
	/bin/bash getcidr.sh cidr.txt

test: all
	./ip2country country-db.bin 8.8.8.8

ls:
	ls -l ip2country country-db.bin cidr.txt; wc -l cidr.txt

clean:
	rm -f ip2country country-db.bin cidr.txt

