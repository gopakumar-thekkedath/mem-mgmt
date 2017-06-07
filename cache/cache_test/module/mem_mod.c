/*
 * Copyright (C) 2017 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/tick.h>
#include <asm/uaccess.h>

#include "mem_ioctl.h"

#define WALK_CNT	128

static int my_major;
static struct cdev mem_cdev;
static uint64_t elapsed_ticks;

static int my_open(struct inode *inode, struct file *file)
{
  printk(KERN_CRIT "\n my_open, fops:%p", file->f_op);
  return 0;
}

static int access_memory(struct mem_access *paccess)
{
	uint64_t *ptr;
	int order = get_order(paccess->mem_size);

	if (order < 0)
		return order;

	ptr = (uint64_t *)__get_free_pages(GFP_KERNEL, order);

	if (!ptr) {
		printk(KERN_ERR "Memory allocation using __get_free_pages failed\n");
		return -1;
	}

	uint64_t tsc = rdtsc();
	
	while (paccess->loop_cnt--) {
	
		uint32_t i;
		volatile uint64_t *tptr = ptr;

		for (i = 0; i < paccess->access_cnt; i++) {
			if (paccess->access_type == ACCESS_READ) {
				uint64_t data = *tptr;
				if (data == 0x5abeef5a)
					printk("\n Yes !!\n");
			} else
				*tptr = 0xdeadbeefUL;
			tptr = (uint64_t *)((uint64_t)tptr + paccess->access_stride);
		
			if (tptr >= (uint64_t *)((uint64_t)ptr + paccess->mem_size))
					tptr = ptr;
		}
	}

	elapsed_ticks = rdtsc() - tsc;
	printk(KERN_CRIT "Ticks=%lu\n", elapsed_ticks);

	free_pages(ptr, order);

	return 0;
}

static long  my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	struct mem_access access;

	switch (cmd) {
	case MEM_ACCESS:
		printk(KERN_CRIT "ioctl : MEM_ACCESS\n");
		if ((ret = copy_from_user(&access, (void __user *)arg,
			sizeof(struct mem_access))) != 0) {
			printk(KERN_ERR "\n Copy from user failed ");
			return ret;
		}
		printk(KERN_CRIT "Size to allocate:%x\n", access.mem_size);
		ret = access_memory(&access);
		break;
	case READ_TICKS:
		printk(KERN_CRIT "ioctl : READ_TICKS\n");
		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))) {
				put_user(elapsed_ticks, (uint64_t  __user *)arg);
				ret = 0;
			} else
				printk(KERN_ERR "access_ok failed\n");
		} else
			printk(KERN_ERR "\nDirection is NOT read\n");
		break;   
 
	default:
      printk(KERN_ERR "\n Unsupported ioctl");
	}

	return ret;
}

static const struct file_operations my_fops = {
  .owner          = THIS_MODULE,
  .unlocked_ioctl = my_ioctl,
  .open           = my_open,
};

static int __init mem_start(void) 
{
 
  int ret; 
  dev_t devid;

  if (my_major) {
    devid = MKDEV(my_major, 0);
    printk(KERN_CRIT "register_chrdev_region\n");
    ret = register_chrdev_region(devid, 1, "mem_module");
  } else {
    printk(KERN_CRIT "alloc_chrdev_region\n");
    ret = alloc_chrdev_region(&devid, 0, 1, "mem_module");
    my_major = MAJOR(devid);
  }

  if (ret < 0) {
      printk(KERN_ERR "\nDUMMY IOCTL character driver reg failed");
      return -1;
  }

  printk(KERN_CRIT "\n Major Number=%u my_fops:%p", my_major, &my_fops);

  cdev_init(&mem_cdev, &my_fops);
  cdev_add(&mem_cdev, devid, 1);

  return 0;
}

static void __exit mem_end(void) 
{
  cdev_del(&mem_cdev);
  unregister_chrdev_region(MKDEV(my_major, 0), 1);
}

module_init(mem_start);
module_exit(mem_end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Seclearn");
MODULE_DESCRIPTION("Test Driver");

