# ip2country

for example:

```sh
[opc@handson-vm01 bin]$ make
/bin/bash getcidr.sh cidr.txt
/bin/bash mkdb.sh cidr.txt country-db.bin
162987 records writed
cc -Wall -O2 -o ip2country main.c fbsrch.c
/usr/bin/strip ip2country
[opc@handson-vm01 bin]$ ./ip2country country-db.bin 8.8.8.8
US
```
