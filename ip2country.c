#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define ERRMSG(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
//#define DEBUG(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
#define DEBUG(s, ...)

/*
00000018: 0100 0200 0100 03ff 1700 434e  ..........CN
00000024: 0100 0400 0100 07ff 1600 4155  ..........AU
00000030: 0100 0800 0100 0fff 1500 434e  ..........CN
*/

/*
cc -Wall -O0 -o ip2country -g ip2country.c && ./ip2country country-db.bin 8.8.8.8 ; rc=$?; echo; echo result is $rc
*/

typedef struct cdbrec_st
{
	uint32_t network;
	uint32_t broadcast;
	uint8_t mask;
	char x;
	char code[2];
}
cdbrec_t;

static void print_addr(const char* k, uint32_t v);

static int file_bsearch(
	FILE* fp, size_t recsize, size_t left, size_t remaining,
	int (*cmpfunc)(const unsigned char*, size_t, const void*),
	const void*, int* pcalled);

static int cmp_cdbrec(const unsigned char* rec, size_t recsize, const void* param);

int main(int argc, char** argv)
{
	int f_result = 1;

	FILE* fp = NULL;

	if (argc != 3)
	{
		ERRMSG("Usage: %s country-db.bin ip-address\n", argv[0]);
		goto LABEL_ERR;
	}

	uint8_t addr_u8s[4];

	if (sscanf(argv[2], "%hhu.%hhu.%hhu.%hhu",
			&addr_u8s[0], &addr_u8s[1], &addr_u8s[2], &addr_u8s[3]) != 4)
	{
		ERRMSG("ERR: parse %s\n", argv[2]);
		goto LABEL_ERR;
	}

/*
	for (int i=0; i<(sizeof(addr_u8s) / sizeof(addr_u8s[0])); i++)
	{
		int n = (int)addr_u8s[i];
		if (n < 0 || 255 < n)
		{
			ERRMSG("ERR: addr[%d] is %d\n", i, n);
			goto LABEL_ERR;
		}
	}
*/

	uint32_t addr = ntohl(*((uint32_t*)addr_u8s));
	DEBUG("INFO: input file=%s addr=%u(0x%x)\n", argv[1], addr, addr);

	fp = fopen(argv[1], "rb");
	if (! fp)
	{
		ERRMSG("ERR: can not open %s\n", argv[1]);
		goto LABEL_ERR;
	}

	struct stat stbuf;
	if (fstat(fileno(fp), &stbuf) != 0)
	{
		ERRMSG("ERR: stat %s\n", argv[1]);
		goto LABEL_ERR;
	}

	if (!stbuf.st_size)
	{
		ERRMSG("ERR: empty data %s\n", argv[1]);
		goto LABEL_ERR;
	}

	size_t remaining = stbuf.st_size / sizeof(cdbrec_t);
	int expected = remaining * sizeof(cdbrec_t);

	if (stbuf.st_size != expected)
	{
		ERRMSG("ERR: illegal record size\n");
		goto LABEL_ERR;
	}

	DEBUG("INFO: sizeof(cdbrec_t)=%zu expected=%d\n", sizeof(cdbrec_t), expected);

	int called = 0;
	f_result = file_bsearch(fp, sizeof(cdbrec_t), 0, remaining, cmp_cdbrec, &addr, &called);

	DEBUG("INFO: file_bsearch() return %d\n", f_result);

LABEL_ERR:

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	DEBUG("\nINFO: done.\n");

	return f_result;
}

void print_addr(const char* k, uint32_t v)
{
	DEBUG("ADDR: %s=", k);
	DEBUG("%u.%u.%u.%u\n", (v>>24)&0xff, (v>>16)&0xff, (v>>8)&0xff, v&0xff);
}


// for buf
#define MAX_CALL	(1000)

int file_bsearch(
	FILE* fp, size_t recsize, size_t left, size_t remaining,
	int (*cmpfunc)(const unsigned char*, size_t, const void*),
	const void* param, int* pcalled)
{
	int f_result = 1;

	DEBUG("\nINFO: file_bsearch(%p, recsize=%zu left=%zu remaining=%zu called=%d)\n",
		fp, recsize, left, remaining, *pcalled);

	unsigned char rec[recsize];

#if defined(MAX_CALL)
	DEBUG("INFO: called=%d\n", *pcalled);

	if (++(*pcalled) > MAX_CALL)
	{
		ERRMSG("ERR: called=%d\n", *pcalled);
		goto LABEL_EXIT;
	}
#endif

	if ((ssize_t)remaining <= 0)
	{
		ERRMSG("ERR: no data\n");

		goto LABEL_EXIT;
	}

	const size_t center = left + (remaining / 2);
	const off_t offset = center * recsize;

	DEBUG("INFO: center=%zu offset=%jd\n", center, (intmax_t)offset);

	if (fseeko(fp, offset, SEEK_SET) != 0)
	{
		ERRMSG("ERR: seek off=%jd\n", (intmax_t)offset);
		goto LABEL_EXIT;
	}

	if (fread(&rec, recsize, 1, fp) != 1)
	{
		ERRMSG("ERR: read off=%jd\n", (intmax_t)ftello(fp));
		goto LABEL_EXIT;
	}

	size_t next_left, next_remaining;

	switch (cmpfunc(rec, recsize, param))
	{
		case 0:
		{
			f_result = 0;

			goto LABEL_EXIT;
		}
		case -1:
		{
			DEBUG("INFO: next direction LEFT\n");

			next_left = left;
			next_remaining = center - left;

			break;
		}
		case 1:
		{
			DEBUG("INFO: next direction RIGHT\n");

			next_left = center + 1;
			next_remaining = (left + remaining) - next_left;

			break;
		}
	}

	DEBUG("INFO: next [left=%zu remaining=%zd]\n", next_left, (ssize_t)next_remaining);

	if ((ssize_t)next_remaining > 0)
	{
		f_result = file_bsearch(fp, recsize, next_left, next_remaining, cmpfunc, param, pcalled);
	}
	else
	{
		ERRMSG("WARN: no data(next)\n");

		f_result = 2;
	}

LABEL_EXIT:

	return f_result;
}

int cmp_cdbrec(const unsigned char* rec, size_t recsize, const void* param)
{
	uint32_t addr = *((uint32_t*)param);
	const cdbrec_t* cdbrec = (cdbrec_t*)rec;

	uint32_t network = ntohl(cdbrec->network);
	uint32_t broadcast = ntohl(cdbrec->broadcast);

	DEBUG("INFO: cdbrec [network=0x%x broadcast=0x%x mask=%d country='%.2s']\n",
		network, broadcast, cdbrec->mask ,cdbrec->code);

	print_addr("network", network);
	print_addr("broadcast", broadcast);

	if (addr < network)
	{
		return -1;
	}
	else if (broadcast < addr)
	{
		return 1;
	}

	printf("%.2s\n", cdbrec->code);

	return 0;
}

