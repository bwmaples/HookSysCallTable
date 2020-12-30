
/*  
 *  hello-1.c - The simplest kernel module.
 */


#include <linux/module.h>	/* Needed by all modules */
#include <linux/init.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include "monitor.h"
// #include <asm/tlbflush.h>

static struct {
	pmd_t *pmd;
	pte_t *pte;
	pmd_t saved_pmd;
	pte_t saved_pte;
	bool made_writeable;
} mem_unprotect;


static inline void m_flush_tlb_all(void)
{
	dsb(ishst);
	asm("tlbi	vmalle1is");
	dsb(ish);
	isb();
}


static inline void __m_flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	if (msm8994_needs_tlbi_wa()) {
		asm("tlbi	vmalle1is");
		dsb(sy);
		isb();
	} else {
		unsigned long addr;
		start >>= 12;
		end >>= 12;

		dsb(ishst);
		for (addr = start; addr < end; addr += 1 << (PAGE_SHIFT - 12))
			asm("tlbi vaae1is, %0" : : "r"(addr));
		dsb(ish);
		isb();
	}
}


static inline void m_flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	if ((end - start) <= MAX_TLB_RANGE)
		__m_flush_tlb_kernel_range(start, end);
	else
		m_flush_tlb_all();
}


void m_mem_text_address_writeable(u64 addr)
{
	printk(KERN_INFO "@bwmaples: Here 0 0...\n");

	// printk(KERN_INFO "@bwmaples: Here 0...\n");
	pgd_t *pgd = m_pgd_offset_k(addr);
	printk(KERN_INFO "@bwmaples: Here 0.1...\n");
	pud_t *pud = pud_offset(pgd, addr);
	u64 addr_aligned;
	printk(KERN_INFO "@bwmaples: Here 1...\n");
	mem_unprotect.made_writeable = 0;
	printk(KERN_INFO "@bwmaples: Here 2...\n");
	mem_unprotect.pmd = pmd_offset(pud, addr);
	addr_aligned = addr & PAGE_MASK;
	mem_unprotect.saved_pmd = *mem_unprotect.pmd;
	printk(KERN_INFO "@bwmaples: Here 3...\n");
	if ((mem_unprotect.saved_pmd & PMD_TYPE_MASK) == PMD_TYPE_SECT) {
		printk(KERN_INFO "@bwmaples: if...\n");
		set_pmd(mem_unprotect.pmd,
			__pmd(__pa(addr_aligned) | m_prot_sect_kernel));
	} else {
		printk(KERN_INFO "@bwmaples: else...\n");			
		mem_unprotect.pte = pte_offset_kernel(mem_unprotect.pmd, addr);
		mem_unprotect.saved_pte = *mem_unprotect.pte;
		set_pte(mem_unprotect.pte, pfn_pte(__pa(addr) >> PAGE_SHIFT,
						   PAGE_KERNEL_EXEC));
	}
	m_flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
	printk(KERN_INFO "@bwmaples: Here 4...\n");
	mem_unprotect.made_writeable = 1;
}


asmlinkage long m_unlink_32(const char __user *pathname)
{
	printk(KERN_INFO "@bwmaples: Origin sys_unlink_32 called path name is %s\n",pathname);
	return origin_unlink_32(pathname);
}


asmlinkage long m_unlinkat_32 (int dfd, const char __user * pathname, int flag)
{
	printk(KERN_INFO "@bwmaples: Origin sys_unlinkat_32 called path name is %s\n",pathname);
	char fileName[1024] = {0};
	copy_from_user(fileName,pathname,256);
	if (strstr(fileName,"******")!=0)
	{
		printk(KERN_INFO "@bwmaples: ******* Found!\n");
		return 0;
	}
	return origin_unlinkat_32(dfd,pathname,flag);

}


static int __init minit_module(void)
{
	printk(KERN_INFO "@bwmaples: Process Start...\n");
	origin_unlink_32 = compat_syscall[__COMPAT_NR_unlink];
	origin_unlinkat_32 = compat_syscall[__COMPAT_NR_unlinkat];
	m_mem_text_address_writeable((uint64_t)compat_syscall);
	compat_syscall[__COMPAT_NR_unlink] = (void*)m_unlink_32;
	compat_syscall[__COMPAT_NR_unlinkat] = (void*)m_unlinkat_32;
	printk(KERN_INFO "@bwmaples: Replace Finish!\n");
	/* 
	 * A non 0 return means init_module failed; module can't be loaded. 
	 */
	return 0;
}


static void  __exit mcleanup_module(void)
{
	printk(KERN_INFO "Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");

module_init(minit_module);
module_exit(mcleanup_module);
