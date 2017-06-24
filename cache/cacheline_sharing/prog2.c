/*
	This program is intended to bring out the usefulness of having
	frequently used data stored in the same cacheline.
	
	We have two structures, one where variables x and y are in the
	same cacheline and the otherone where they are in different cache
	lines. 

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
	
struct cacheline_separate {
	unsigned int x;	
	unsigned char pad[4096];
	unsigned int y;
}__attribute__((aligned(64))); 
	
struct cacheline_same {
	unsigned int x;	
	unsigned int y;
	unsigned char pad[64];
}__attribute__((aligned(64))); 

struct cacheline_separate c_sep;	
struct cacheline_same c_same;

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
	//asm volatile("clflush %0" : :"m"(*((volatile uint8_t *)ptr1)));
	asm volatile("clflush %0": "+m"(*((volatile uint8_t *)ptr1))); ////this is the right usage and should take more time
}

/*
 * In this routine, we will access the variables, idea is to show
 * that by keeping frequently used variables together so that they
 * are in the same cacheline can give performance benefits.
 * We will flush the cachelines explicitly here to simulate the 
 * scenario that between each access of these data enough processing
 * could have happened to flush the data out of cache.
 */
static void access_memory(int opt)
{
	volatile uint32_t *x, *y;
	volatile uint32_t *t1, *t2;
	uint64_t count = 0x10000;
	int val;
	uint64_t time1, time2;	

	t1 = malloc(100);
	t2 = t1;	
		
	if (opt == 1) {
		x = &c_same.x;
		y = &c_same.y;
	} else {
		x = &c_sep.x;
		y = &c_sep.y;	
	}

	printf("\n Read from address:%lx and %lx\n", x, y);
	time1 = rdtsc();

	while (count--) {
		*x += 100;
		*y += *x;
		//asm volatile("mfence" : : :"memory");
		flush_cacheline(x); 
		flush_cacheline(y);
		//asm volatile("pause" : : : "memory"); 
	}
	time2 = rdtsc();

	printf("\nTime spent:%lu ticks\n", time2 - time1);
}	

int 
main(int argc, char *argv[])
{
		
	if (argc < 2) {
		printf("\nUsage, prog1 1/2");
		printf("\n 1 = same cacheline, 2 = different cacheline");
		return -1;
	}
	
	access_memory(atoi(argv[1]));	
 
    exit(EXIT_SUCCESS);
}
	 
