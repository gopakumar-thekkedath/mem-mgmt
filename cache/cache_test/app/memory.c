/*
 * The code here basically try to bring out some of the behaviors
 * that are dependent on how the cache and memory is organized.
 */
#define _GNU_SOURCE /* For cpu affinity related routines */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sched.h>


#include "../module/mem_ioctl.h"

struct mem_access mem_access;

#define MAX_CACHE_LEVEL	3
enum cache_types {
	L1 = 0,
	L2,
	L3,
	DONE,
};

#define PAGE_SIZE	4096
#define ARA_SIZE	PAGE_SIZE

static uint8_t memory[ARA_SIZE];
#define MEM_ACCESS_LOOP	1000000
struct mem_walker {
	void *ptr;
	uint32_t num_bytes;
	int8_t cpu;
	uint64_t ticks;
};

#define MAX_CORES	128

/* this structure should be populated dynamically */
struct core_info {
	int8_t cpu_num;
	int8_t sibling_num;
};

static struct core_info core_info[MAX_CORES];

/* data cache info */
struct cache_struct {
	uint8_t type;
	uint8_t cacheline_size;
	uint32_t cache_size;
	uint8_t ways;
	uint16_t sets;
};

struct cache_struct cstruct[DONE - 1];

static uint64_t
rdtsc(void)
{
    uint32_t tsc_l, tsc_h;

    asm volatile("rdtsc\n\t"
                :"=a"(tsc_l), "=d"(tsc_h)
                ::"memory");

    return ((uint64_t)(tsc_h) << 32) | tsc_l;
}

static uint64_t memory_access(volatile uint32_t *ptr, uint32_t num_bytes,
						  uint32_t loop, bool read)
{
	uint32_t i, j;
	uint64_t data = 0;
	uint64_t tsc1, tsc2;

	num_bytes /= sizeof(uint32_t);

	printf("\n Access %u bytes from from %p\n", num_bytes, ptr);
 
	tsc1 = rdtsc();
	for (i = 0; i < loop; i++) {
		for (j = 0; j < num_bytes; j++) {
			if (read == true)
				data = *(ptr + j);
			else
				*(ptr + j) = 0xdeadbeef;
		}
	}

	tsc2 = rdtsc();
	
	//printf("\ndata=%lu\n ticks:%lu", data, tsc2 - tsc1);

	return (tsc2 - tsc1);
}

/*
 * Remove hardcoding, populate this at run time
 */
static void init_memaccess()
{
	cstruct[L1].type = L1;
	cstruct[L1].cacheline_size = 64U;
	cstruct[L1].cache_size = 32U * 4096;
	cstruct[L1].ways = 8;
	cstruct[L1].sets = 64;
	
	cstruct[L2].type = L2;
	cstruct[L2].cacheline_size = 64U;
	cstruct[L2].cache_size = 256U * 4096;
	cstruct[L2].ways = 8;
	cstruct[L2].sets = 512;
	
	cstruct[L3].type = L3;
	cstruct[L3].cacheline_size = 64U;
	cstruct[L3].cache_size = 3072U * 4096;
	cstruct[L3].ways = 12;
	cstruct[L3].sets = 4096;

}

/* XXX: Remove hardcoding populate this at runtime */
static void init_core_info(void)
{
	core_info[0].cpu_num = 0;
	core_info[0].sibling_num = 1;
	
	core_info[1].cpu_num = 1;
	core_info[1].sibling_num = 0;
	
	core_info[2].cpu_num = 2;
	core_info[2].sibling_num = 3;
	
	core_info[3].cpu_num = 3;
	core_info[3].sibling_num = 2;
}

static void l1_access(int fd, int mode, uint32_t stride)
{
	#define MEM_ACCESS_CNT	32	
	#define LOOP_CNT	1000	
	mem_access.loop_cnt = LOOP_CNT;
	mem_access.access_stride = stride;
	mem_access.access_cnt = MEM_ACCESS_CNT;
	mem_access.mem_size = mem_access.access_stride * mem_access.access_cnt;
	mem_access.access_type = mode;
	printf("\n Access stride:%x Access cnt:%x Access stride * Access cnt=%x\n", 
			mem_access.access_stride, mem_access.access_cnt, 
			mem_access.access_stride * mem_access.access_cnt);
	printf("\n Memory size for L1 access:%x\n", mem_access.mem_size); 
	
	if (ioctl(fd, MEM_ACCESS, &mem_access) == 0) {
		uint64_t ticks;
		if (ioctl(fd, READ_TICKS, &ticks) == 0) { 
			printf("\n Time elapsed =%lu ticks\n", ticks);
		} else
			printf("\n ioctl READ_TICKS failed !!\n");
	} else 
		printf("\n ioctl MEM_ACCESS failed !!\n");
}

/*
 * The test works as below
 * The intend is to access contiguous memory in a single thread  such that 
 * it would bring-out the difference in access speed when data is in L1, L2, L3
 *  etc
 *
 * Note: No attempt is made intentionally to access memory/cache from other 
 * nodes/cores.
 * 1) L1 Access
 *	Here we will access memory such that it would be cached and any repeated
 *	access of the address would fetch it from cache. Hence the time spend 
 *  inside the loop is only for L1 access (except for the initial run when the
 *	addresses are not cached yet)

 * 2) L2 Access
 *	Here the access would be such that, the various addresses should hit the 
 *	same cache line. For example, considring L1 cache with 8 ways, each way
 *  having 64 sets of 64 byte cache lines (hence one way = 4096 bytes). We 
 *  will generate addresses such as 0x0, 0x8000, 0x40000 etc which all refers
 *  to the same cache line thereby forcing L1 cache flush and access from L2
 *
 *  The same logic can be applied to L3 and main memory
 *
 * For memory reads, a collision in the cache would result in the earlier data
 * being discarded as it is not dirty, hence we do not expect to see much 
 * difference but for writes, as the dirty data needs to be written to memory
 * before new data comes in to the cache line, we should see some difference
 */
static void access_latency_test(int fd, int mode, uint32_t stride)
{
	l1_access(fd, mode, stride);
  
}

static int get_core(int cpu_num, bool is_ht)
{
	int i;
	int n = sizeof(core_info) / sizeof(struct core_info);

	for (i = 0; i < n; i++) {
		if (is_ht == true) {
			if (core_info[i].cpu_num == cpu_num)
				return core_info[i].sibling_num;
		} else {
			if (core_info[i].cpu_num != cpu_num)
				if (core_info[i].sibling_num != cpu_num)
					return core_info[i].cpu_num;
			}
	}

	return -1;
}


static void *thread_1(void *ptr)
{
	cpu_set_t my_set;
	struct mem_walker *mem_walk;
	
	mem_walk = (struct mem_walker *)ptr;	
	
	CPU_ZERO(&my_set);
	CPU_SET(mem_walk->cpu, &my_set);
	
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
	mem_walk->ticks = memory_access(mem_walk->ptr, mem_walk->num_bytes,
				 MEM_ACCESS_LOOP , false);
}

static void *thread_2(void *ptr)
{
	cpu_set_t my_set;
	struct mem_walker *mem_walk;
	
	mem_walk = (struct mem_walker *)ptr;	
    
	CPU_ZERO(&my_set);
	CPU_SET(mem_walk->cpu, &my_set);
	
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
	mem_walk->ticks = memory_access(mem_walk->ptr, mem_walk->num_bytes,
				  MEM_ACCESS_LOOP, false);
}

static int launch_threads(int8_t cpu1, int8_t cpu2, uint32_t size)
{
	pthread_t thread1, thread2, thread3, thread4;
	int  iret;
	struct mem_walker mem_walk[2];
	uint64_t tick1 = rdtsc(), tick2;

	if (cpu1 >= 0) {
		mem_walk[0].cpu = cpu1;
		mem_walk[0].ptr = &memory[0];
		mem_walk[0].ticks = 0; 
		mem_walk[0].num_bytes = size;
	
		iret = pthread_create( &thread1, NULL, thread_1, (void*)&mem_walk[0]);
		if (iret) {
			fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
			return -1; 
		}
	}

	if (cpu2 >= 0) {
		mem_walk[1].cpu = cpu2;
		mem_walk[1].ptr = &memory[ARA_SIZE / 2];
		mem_walk[1].ticks = 0;
		mem_walk[1].num_bytes = size;
	
		iret = pthread_create( &thread2, NULL, thread_2, (void*)&mem_walk[1]);
		if (iret) {
			fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
			return -1;
		}
	}

	if (cpu1 >= 0)
		pthread_join( thread1, NULL);
	
	if (cpu2 >= 0)
		pthread_join( thread2, NULL);

	tick2 =rdtsc();
	printf("\n thread 1 ticks in CPU:%d =%lu\n", cpu1,  mem_walk[0].ticks);
	printf("\n thread 2 ticks in CPU:%d =%lu\n", cpu2,  mem_walk[1].ticks);
	printf("\n total ticks :%lu\n", tick2 - tick1);

}

static void seq_mem_access_single_core(int8_t cpu)
{
	uint64_t ticks;
	launch_threads(cpu, -1, ARA_SIZE);
}

static void seq_mem_access_dual_cores(bool is_ht)
{
	int8_t cpu1 = core_info[0].cpu_num;
	int8_t cpu2;

	cpu2 = get_core(cpu1, is_ht);

	if (cpu2 < 0)  
		return;

	if (is_ht == true) {
		printf("\n Seq memory access test using hyperthreaded cores\n");
	} else {
		printf("\n Seq memory access test using non hyperthreaded cores\n");
	}

	printf("\n CPUs:%u and %u\n", cpu1, cpu2);

	/* launch the threads in the selected CPUs */
	launch_threads(cpu1, cpu2, ARA_SIZE / 2); 
}


void main(void)
{
	int i, opt;
	int fd = open("/dev/mem_mod", O_RDWR);

	if (fd < 0) { 
		printf("\n Unable to open device file, /dev/dummy_module!!\n");
		//return;
	}

	init_core_info();

	while (true) {
		if (fd >= 0) {
			printf("\n 1. Memory Read With L1 hits(unithread)");
			printf("\n 2. Memory Write with L1 hits(unithread)");	
			printf("\n 3. Memory Read With L1 collision(unithread)");
			printf("\n 4. Memory Write with L1 collision(unithread)");
		}
		/*
		 * Idea behind the tests below is to access memory in a loop, and
		 * measure the performance. We will access memory of ARA_SIZE in
		 * one core as well as divide and access using two cores, hyperthreaded
		 * and non hyperthreaded cores. We do not expect single core access time
		 * to exceed multi core for obvious reasons. More interesting will be 
		 * the hyper-threaded vs non hyper-threaded core performances. Hyper-
		 * threaded cores will suffer due to sharing of computational resources
		 * and we will bring out this here.
		 * Finally, another test is to isolate a core, run the rest of 
		 * the cores with high CPU load and then compare the performances of 
		 * single and dual cores.
		 *	Interestingly, in this case, though i had 100s of threads continously
		 *  hogging available CPUs, there is not much of difference in time seen
		 *  between below tests, scheduing on non isolated cores vs isolated cores.
		 * a) This was because, we had isolated only one hyperthread,ie 3, but
		 *    its sibling 2 was hogged, hence pinning 3 did not help as 2 was busy
		 *	  and was taking away the resources. On isolating both 2 and 3, better
		 *	  performance were seen as compared to 0 and 1.
		 */
		printf("\n 5. Seq memory access, 2 cores, Hyperthreaded"); 
		printf("\n 6. Seq memory access, 2 cores, not HT");
		printf("\n 7. Seq memory access, single core"); 
		printf("\n 0. Exit");

		printf("\nEnter your option:");
		scanf("%d", &opt);
		init_memaccess();

		switch (opt) {
		case 1:
			access_latency_test(fd, ACCESS_READ, 64);
			break;
		case 2:
			access_latency_test(fd, ACCESS_WRITE, 64);
			break;
		case 3: 
			access_latency_test(fd, ACCESS_READ, 4096);
			break;
		case 4:
			access_latency_test(fd, ACCESS_WRITE, 4096);
			break;
		case 5:
			seq_mem_access_dual_cores(true);
			break;
		case 6:
			seq_mem_access_dual_cores(false);
			break;
		case 7:
		{
			uint8_t core;
			printf("\n Provide core number:");
			scanf("%d", &core);
			seq_mem_access_single_core(core);
			break;
		}
		case 0:
			exit(0);
		default:
			printf("\n Unsupported Option");
		}
	}


	close(fd);
}
