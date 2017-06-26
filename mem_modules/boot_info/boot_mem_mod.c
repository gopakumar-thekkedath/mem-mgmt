/*
 * Copyright (C) 2017 
 */

#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/kallsyms.h>

static int __init mem_start(void) 
{
	unsigned long max_low_pfn_addr, min_low_pfn_addr, max_pfn_addr;

	max_low_pfn_addr = kallsyms_lookup_name("max_low_pfn");
	min_low_pfn_addr = kallsyms_lookup_name("min_low_pfn");
	max_pfn_addr = kallsyms_lookup_name("max_pfn");
	
	if (max_low_pfn_addr) 
		printk(KERN_CRIT "max_low_pfn:%lu", *((unsigned long *)max_low_pfn_addr));
	
	if (min_low_pfn_addr)
		printk(KERN_CRIT "min_low_pfn_addr:%lu", *((unsigned long *)min_low_pfn_addr));

	if (max_pfn_addr) 
		printk(KERN_CRIT "max_pfn:%lu\n", *((unsigned long *)max_pfn_addr));
 
	return 0;
}

static void __exit mem_end(void) 
{
}

module_init(mem_start);
module_exit(mem_end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Seclearn");
MODULE_DESCRIPTION("Boot memory info gatherer");

