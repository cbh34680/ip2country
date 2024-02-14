#include <stdint.h>
#include "fbsrch.h"


#define ERRMSG(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
//#define DEBUG(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
#define DEBUG(s, ...)

// for bug
#define MAX_CALL	(1000)


int file_bsearch(
	FILE* fp, size_t recsize, size_t left, size_t remaining,
	int (*cmpfunc)(const unsigned char*, size_t, void*),
	void* param, int* pcalled)
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

	size_t next_left = 0, next_remaining = 0;

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
		DEBUG("WARN: not found\n");

		f_result = 2;
	}

LABEL_EXIT:

	return f_result;
}

