//===------------------ main.cc - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gcalloc.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

unsigned int rand(unsigned int min, unsigned int max) {
	return (unsigned int)nearbyint((((double)(max - min))*::rand())/(RAND_MAX + 1.0)) + min;
}

void tire(size_t n, size_t *vals, size_t *reallocs, size_t *frees) {
	for(size_t i=0; i<n; i++) {
		if(rand(0, 20))
			vals[i] = rand(1, 256);
		else
			vals[i] = rand(256, 65536);
		if(!rand(0, 10)) /* un chance sur 10 de réallouer */
			if(rand(0, 20))
				reallocs[i] = rand(1, 256);
			else
				reallocs[i] = rand(256, 65536);
		else
			reallocs[i] = 0;
		if(rand(0, 10)) /* un chance sur 10 de ne pas libérer */
			frees[i] = 1;
		else
			frees[i] = 0;
	}
}

void printMesure(size_t n, size_t *vals, size_t *reallocs, size_t *frees, struct timeval *start, struct timeval *end) {
	size_t   totAllocated = 0;
	size_t   curAllocated = 0;
	size_t   na = 0;
	size_t   nr = 0;
	size_t   nf = 0;
	size_t   no = 0;

	for(size_t i=0; i<n; i++) {
		size_t here = vals[i] + reallocs[i];
		if(reallocs[i]) {
			na++;
			nr++;
		}
		na++;
		totAllocated += here;
		if(!frees[i]) {
			curAllocated += here;
			no++;
		} else
			nf++;
	}

	printf("; %d allocations (%d reallocations, %d free, %d chunks)\n", na, nr, nf, no);
	printf(";   Allocated: %d bytes, current %d bytes\n", totAllocated, curAllocated);
	
	struct timeval diff;
	timersub(end, start, &diff);
	double f = (double)diff.tv_sec + ((double)diff.tv_usec / 1000000.0);
	printf(";   time consumed: %fs\n", f);
}

void mesureAllocateur() {
	size_t n = 512*1024;
	size_t vals[n];
	size_t reallocs[n];
	size_t frees[n];

	printf("Tire une sequence d'allocations alléatoire\n");
	tire(n, vals, reallocs, frees);

	printf("test\n");
	struct timeval start;
	struct timeval end;
	GCAllocator *a = new GCAllocator();

	gettimeofday(&start, 0);
	for(size_t i=0; i<n; i++) {
		void *p = a->alloc(vals[i]);
		if(reallocs[i])
			p = a->realloc(p, reallocs[i]);
		if(frees[i])
			a->free(p);
	}
	gettimeofday(&end, 0);
	delete a;

	printMesure(n, vals, reallocs, frees, &start, &end);
	printf("Press a key...\n");
	getchar();

	gettimeofday(&start, 0);
	for(size_t i=0; i<n; i++) {
		void *p = malloc(vals[i]);
		((char *)p)[0] = 1;
		if(reallocs[i])
			p = realloc(p, reallocs[i]);
		if(frees[i])
			free(p);
	}
	gettimeofday(&end, 0);
	printMesure(n, vals, reallocs, frees, &start, &end);
	printf("Press a key...\n");
	getchar();

}

int main(int argc, char **argv) {
	mesureAllocateur();

	GCHash::destroy();
	return 0;
}
