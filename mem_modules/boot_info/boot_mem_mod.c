/*
 * Copyright (C) 2017 
 */

#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/mmzone.h>
#include <linux/kallsyms.h>


static int __init mem_start(void) 
{
	unsigned long max_low_pfn_addr, min_low_pfn_addr, max_pfn_addr;
	unsigned long max_possible_pfn_addr, max_low_pfn_mapped_addr;
	unsigned long max_pfn_mapped_addr;
	struct pglist_data *pgdat;

	max_low_pfn_addr = kallsyms_lookup_name("max_low_pfn");
	min_low_pfn_addr = kallsyms_lookup_name("min_low_pfn");
	max_pfn_addr = kallsyms_lookup_name("max_pfn");
	max_possible_pfn_addr = kallsyms_lookup_name("max_possible_pfn");
	max_low_pfn_mapped_addr = kallsyms_lookup_name("max_low_pfn_mapped");
	max_pfn_mapped_addr = kallsyms_lookup_name("max_pfn_mapped");
	pgdat = kallsyms_lookup_name("node_data");

	printk(KERN_CRIT "pgdat as got from symtab:%lx\n", (unsigned long)pgdat);
	pgdat = NODE_DATA(0);
	printk(KERN_CRIT "pgdat as got from NODE_DATA:%lx\n", (unsigned long)pgdat);
	
	if (max_low_pfn_addr) 
		printk(KERN_CRIT "max_low_pfn:%lu", *((unsigned long *)
				max_low_pfn_addr));
	
	if (min_low_pfn_addr)
		printk(KERN_CRIT "min_low_pfn_addr:%lu", *((unsigned long *)
				min_low_pfn_addr));

	if (max_pfn_addr) 
		printk(KERN_CRIT "max_pfn:%lu\n", *((unsigned long *)max_pfn_addr));
	
	if (max_possible_pfn_addr) 
		printk(KERN_CRIT "max_possible_pfn:%lu\n", *((unsigned long *)
				max_possible_pfn_addr));
	
	if (max_low_pfn_mapped_addr) 
		printk(KERN_CRIT "max_low_pfn_mapped:%lu\n", *((unsigned long *)
				max_low_pfn_mapped_addr));
	
	if (max_pfn_mapped_addr) 
		printk(KERN_CRIT "max_pfn_mapped:%lu\n", *((unsigned long *)
				max_pfn_mapped_addr));

	if (pgdat) 
		printk(KERN_CRIT "node_start_pfn:%lu node_present_pages:%lu"
				" node_spanned_pages:%lu", pgdat->node_start_pfn,
				pgdat->node_present_pages, pgdat->node_spanned_pages);			

	else
		printk(KERN_CRIT "Unable to spot node_data\n");	
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

