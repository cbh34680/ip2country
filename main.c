#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "fbsrch.h"

#define ERRMSG(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
//#define DEBUG(s, ...) fprintf(stderr, s, ##__VA_ARGS__)

#if !defined(DEBUG)
  #define DEBUG(s, ...)
#endif

/*
 * country-db.bin
 *
00000018: 0100 0200 0100 03ff 1700 434e  ..........CN
00000024: 0100 0400 0100 07ff 1600 4155  ..........AU
00000030: 0100 0800 0100 0fff 1500 434e  ..........CN
*/

/*
cc -Wall -O0 -g -o ip2country main.c fbsrch.c && ./ip2country country-db.bin 8.8.8.8 ; rc=$?; echo result is $rc
*/

typedef struct ccdbrec_st
{
	uint32_t network;
	uint32_t broadcast;
	uint8_t mask;
	char x;
	char code[2];
}
ccdbrec_t;

static int cmp_ccdbrec(const unsigned char* rec, size_t recsize, void* param);

static void print_addr(const char* k, uint32_t v);

void print_addr(const char* k, uint32_t v)
{
	DEBUG("ADDR: %s=", k);
	DEBUG("%u.%u.%u.%u\n", (v>>24)&0xff, (v>>16)&0xff, (v>>8)&0xff, v&0xff);
}

typedef struct cmp_param_st
{
	uint32_t addr;
	ccdbrec_t ccdbrec;
}
cmp_param_t;

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

	size_t remaining = stbuf.st_size / sizeof(ccdbrec_t);
	int expected = remaining * sizeof(ccdbrec_t);

	if (stbuf.st_size != expected)
	{
		ERRMSG("ERR: illegal record size\n");
		goto LABEL_ERR;
	}

	DEBUG("INFO: sizeof(ccdbrec_t)=%zu expected=%d\n", sizeof(ccdbrec_t), expected);

	cmp_param_t cmp_param = { .addr = addr };

	int called = 0;
	f_result = file_bsearch(fp, sizeof(ccdbrec_t), 0, remaining, cmp_ccdbrec, &cmp_param, &called);

	DEBUG("INFO: file_bsearch() return %d\n", f_result);

	if (f_result == 0)
	{
		DEBUG("INFO: ccdbrec[network=0x%x broadcast=0x%x mask=%u code=%.2s]\n",
			ntohl(cmp_param.ccdbrec.network),
			ntohl(cmp_param.ccdbrec.broadcast),
			cmp_param.ccdbrec.mask, cmp_param.ccdbrec.code);

		print_addr("network", ntohl(cmp_param.ccdbrec.network));
		print_addr("broadcast", ntohl(cmp_param.ccdbrec.broadcast));

		printf("%.2s\n", cmp_param.ccdbrec.code);
	}

LABEL_ERR:

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	DEBUG("\nINFO: done.\n");

	return f_result;
}

int cmp_ccdbrec(const unsigned char* rec, size_t recsize, void* param)
{
	cmp_param_t* cmp_param = (cmp_param_t*)param;
	const ccdbrec_t* ccdbrec = (ccdbrec_t*)rec;

	uint32_t network = ntohl(ccdbrec->network);
	uint32_t broadcast = ntohl(ccdbrec->broadcast);

	DEBUG("INFO: ccdbrec [network=0x%x broadcast=0x%x mask=%d country='%.2s']\n",
		network, broadcast, ccdbrec->mask ,ccdbrec->code);

	print_addr("network", network);
	print_addr("broadcast", broadcast);

	if (cmp_param->addr < network)
	{
		return -1;
	}
	else if (broadcast < cmp_param->addr)
	{
		return 1;
	}

	cmp_param->ccdbrec = *ccdbrec;

	return 0;
}

