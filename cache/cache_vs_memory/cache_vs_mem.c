/*
 * Time difference between fetching data from cache vs memory 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
	
struct cacheline_separate {
	unsigned int x;	
	unsigned char pad[64];
	unsigned int y;
}__attribute__((aligned(64))); 
	
struct cacheline_separate c_sep;	

static inline uint64_t rdtsc(void)
{	
	uint32_t tsc_l, tsc_h;

	asm volatile("rdtsc\n\t"
		:"=a"(tsc_l), "=d"(tsc_h)
        ::"memory");

	return ((uint64_t)(tsc_h) << 32) | tsc_l;
}

static inline void flush_cacheline(volatile void *ptr1)
{
	asm volatile("clflush %0": "+m"(*((volatile uint8_t *)ptr1))); 
}

/*
 * In this routine, we will access the variable 'x' each time in the loop,
   after which we will call 'clflush', the cacheline that gets flushed depends
   on the user input, it could either be the cacheline that 'x' is part of,
   (in which case next access of 'x' should come from memory) or cacheline
   next to the one holding 'x' (so that the next access of 'x' will continue
   to get satisfied from cache). This would bringout the performance difference
   in accessing a data from cache vs memory
 */
static void access_memory(int opt)
{
	volatile uint32_t *x, *y;
	uint64_t count = 0x10000;
	int val;
	uint64_t time1, time2;	

	x = &c_sep.x;
	y = &c_sep.y;	

	printf("\n Read from address:%lx and %lx\n", x, y);
	time1 = rdtsc();

	while (count--) {
		*x += 100;
		if (opt == 1)
			flush_cacheline(y);
		else
			flush_cacheline(x); 
	}
	time2 = rdtsc();

	printf("\nTime spent:%lu ticks\n", time2 - time1);
}	

int 
main(int argc, char *argv[])
{
		
	if (argc < 2) {
		printf("\n./cache_vs_mem 1/2");
		printf("\n 1 = fetch 'x' from cache, 2 = fetch 'x' from memory");
		return -1;
	}
	
	access_memory(atoi(argv[1]));	
 
    exit(EXIT_SUCCESS);
}
	 
