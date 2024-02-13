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
cc -Wall -O0 -o ip2country -g ip2country.c && ./ip2country cidr.bin 193.34.185.1 ; rc=$?; echo; echo result is $rc
*/

typedef struct cidr_st
{
	uint32_t network;
	uint32_t broadcast;
	uint8_t mask;
	char x;
	char country[2];
}
cidr_t;

static void print_addr(const char* k, uint32_t v);
static int search_data(FILE* fp, uint32_t addr, size_t left, size_t remaining, int* pcalled);

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

	if (sscanf(argv[2], "%hhu.%hhu.%hhu.%hhu", &addr_u8s[0], &addr_u8s[1], &addr_u8s[2], &addr_u8s[3]) != 4)
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

	size_t remaining = stbuf.st_size / sizeof(cidr_t);
	int expected = remaining * sizeof(cidr_t);

	if (stbuf.st_size != expected)
	{
		ERRMSG("ERR: illegal record size\n");
		goto LABEL_ERR;
	}

	DEBUG("INFO: sizeof(cidr_t)=%zu expected=%d\n", sizeof(cidr_t), expected);

	int called = 0;
	f_result = search_data(fp, addr, 0, remaining, &called);

	DEBUG("INFO: search_data() return %d\n", f_result);


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

int search_data(FILE* fp, uint32_t addr, size_t left, size_t remaining, int* pcalled)
{
	int f_result = 1;

	DEBUG("\nINFO: search_data(%p, addr=0x%x left=%zu remaining=%zu called=%d)\n", fp, addr, left, remaining, *pcalled);

	print_addr("search", addr);

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

	size_t center = left + (remaining / 2);
	off_t offset = center * sizeof(cidr_t);

	DEBUG("INFO: center=%zu offset=%jd\n", center, (intmax_t)offset);

	cidr_t rec;

	if (fseeko(fp, offset, SEEK_SET) != 0)
	{
		ERRMSG("ERR: seek off=%jd\n", (intmax_t)offset);
		goto LABEL_EXIT;
	}

	if (fread(&rec, sizeof(rec), 1, fp) != 1)
	{
		ERRMSG("ERR: read off=%jd\n", (intmax_t)ftello(fp));
		goto LABEL_EXIT;
	}

	rec.network = ntohl(rec.network);
	rec.broadcast = ntohl(rec.broadcast);


	DEBUG("INFO: rec [network=0x%x broadcast=0x%x mask=%d country='%.2s']\n", rec.network, rec.broadcast, rec.mask ,rec.country);
	print_addr("network", rec.network);
	print_addr("broadcast", rec.broadcast);

	if (rec.network <= addr && addr <= rec.broadcast)
	{
		printf("%.2s\n", rec.country);
		f_result = 0;
	}
	else
	{
		size_t next_left, next_remaining;

		if (addr < rec.network)
		{
			DEBUG("INFO: next direction LEFT\n");

			next_left = left;
			next_remaining = center - left;
		}
		else if (rec.broadcast < addr)
		{
			DEBUG("INFO: next direction RIGHT\n");

			next_left = center + 1;
			next_remaining = (left + remaining) - next_left;
		}
		else
		{
			ERRMSG("ERR: illegal range\n");
			goto LABEL_EXIT;
		}

		DEBUG("INFO: next [left=%zu remaining=%zd]\n", next_left, (ssize_t)next_remaining);

		if ((ssize_t)next_remaining <= 0)
		{
			ERRMSG("WARN: no data(next)\n");
			f_result = 2;

			goto LABEL_EXIT;
		}

		f_result = search_data(fp, addr, next_left, next_remaining, pcalled);
	}

LABEL_EXIT:

	return f_result;
}

