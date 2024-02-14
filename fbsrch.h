#pragma once

#include <stdio.h>
#include <stddef.h>

extern int file_bsearch(
        FILE* fp, size_t recsize, size_t left, size_t remaining,
        int (*cmpfunc)(const unsigned char*, size_t, void*),
        void*, int* pcalled);

