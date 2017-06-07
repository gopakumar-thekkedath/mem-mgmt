/*
 * Copyright (C) 2017 
 */
#ifndef _DEV_IOCTL_H_
#define _DEV_IOCTL_H_

enum access {
		ACCESS_READ,
		ACCESS_WRITE,
};

struct mem_access {
	uint32_t loop_cnt;
	uint32_t access_stride;
	uint32_t access_cnt;
	uint32_t mem_size; /* we do not expect to allocate more than 4GB */
	uint8_t access_type;
};

#define MEM_ACCESS _IOWR('x', 0x1, struct mem_access)
#define READ_TICKS _IOR('x', 0x2, uint64_t)
#endif /* _DEV_IOCTL_H_ */
