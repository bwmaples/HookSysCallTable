
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


// //写日志内核线程
// int thread_fslog(int start)
// {
//     if(start == 0){do_exit(0);}
//     struct file *fp =filp_open(log_fs.filename,O_CREAT|O_RDWR |O_APPEND|O_SYNC,0666);  //打开文件
//     if (IS_ERR(fp)) //如果打开失败
//     {
//         printk("create file error\n");
//         do_exit(0);
//     }
//     mm_segment_t fs;
//     loff_t pos = 0;
//     fs =get_fs();
//     set_fs(KERNEL_DS);//改变mm_segment, 否则vfs_write函数无法执行
//     vfs_write(fp,&log_fs.buff[start - WRSIZE],WRSIZE, &pos);  //写日志
//     set_fs(fs);
//     filp_close(fp,NULL);  //关闭文件
//     do_exit(0);
// }


// //将日志写入缓冲区
// char add_to_fslog(char *data)
// {
// 	spinlock_t lock;
//     spin_lock(&log_fs.lock);  //防止同时写入缓冲区
//     log_fs.index = (log_fs.index + id)%LOGSIZE;
//     spin_unlock(&log_fs.lock);
//     kthread_run(&thread_fslog,start,"");
//     return 0;
// }

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
	printk(KERN_INFO "@Tsingxing: Here 0 0...\n");

	// printk(KERN_INFO "@Tsingxing: Here 0...\n");
	pgd_t *pgd = m_pgd_offset_k(addr);
	printk(KERN_INFO "@Tsingxing: Here 0.1...\n");
	pud_t *pud = pud_offset(pgd, addr);
	u64 addr_aligned;
	printk(KERN_INFO "@Tsingxing: Here 1...\n");
	mem_unprotect.made_writeable = 0;
	printk(KERN_INFO "@Tsingxing: Here 2...\n");
	mem_unprotect.pmd = pmd_offset(pud, addr);
	addr_aligned = addr & PAGE_MASK;
	mem_unprotect.saved_pmd = *mem_unprotect.pmd;
	printk(KERN_INFO "@Tsingxing: Here 3...\n");
	if ((mem_unprotect.saved_pmd & PMD_TYPE_MASK) == PMD_TYPE_SECT) {
		printk(KERN_INFO "@Tsingxing: if...\n");
		set_pmd(mem_unprotect.pmd,
			__pmd(__pa(addr_aligned) | m_prot_sect_kernel));
	} else {
		printk(KERN_INFO "@Tsingxing: else...\n");			
		mem_unprotect.pte = pte_offset_kernel(mem_unprotect.pmd, addr);
		mem_unprotect.saved_pte = *mem_unprotect.pte;
		set_pte(mem_unprotect.pte, pfn_pte(__pa(addr) >> PAGE_SHIFT,
						   PAGE_KERNEL_EXEC));
	}
	m_flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
	printk(KERN_INFO "@Tsingxing: Here 4...\n");
	mem_unprotect.made_writeable = 1;
}

asmlinkage long m_unlink_32(const char __user *pathname)
{
	printk(KERN_INFO "@Tsingxing: Origin sys_unlink_32 called path name is %s\n",pathname);
	return origin_unlink_32(pathname);
}

asmlinkage long m_unlinkat_32 (int dfd, const char __user * pathname, int flag)
{
	printk(KERN_INFO "@Tsingxing: Origin sys_unlinkat_32 called path name is %s\n",pathname);
	char fileName[1024] = {0};
	copy_from_user(fileName,pathname,256);
	if (strstr(fileName,"GameProtector3")!=0)
	{
		printk(KERN_INFO "@Tsingxing: GameProtector3 Found!\n");
		return 0;
	}
	return origin_unlinkat_32(dfd,pathname,flag);

}


static int __init minit_module(void)
{
	printk(KERN_INFO "@Tsingxing: Process Start...\n");
	origin_unlink_32 = compat_syscall[__COMPAT_NR_unlink];
	origin_unlinkat_32 = compat_syscall[__COMPAT_NR_unlinkat];
	m_mem_text_address_writeable((uint64_t)compat_syscall);
	compat_syscall[__COMPAT_NR_unlink] = (void*)m_unlink_32;
	compat_syscall[__COMPAT_NR_unlinkat] = (void*)m_unlinkat_32;
	printk(KERN_INFO "@Tsingxing: Replace Finish!\n");
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
