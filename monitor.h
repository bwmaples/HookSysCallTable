#include <linux/kernel.h>
#include "syscall32.h"
#define COMPAT_SYSTABLE 0xffffffc00020c000
#define SYSTABLE 0xffffffc001361000
#define MAX_TLB_RANGE	(1024UL << PAGE_SHIFT)



void ** syscall = (void**)SYSTABLE;
void ** compat_syscall = (void**)COMPAT_SYSTABLE;
struct mm_struct *m_init_mm = (struct mm_struct *)0xffffffc00197bb88;

#define m_pgd_offset_k(addr)	pgd_offset(m_init_mm, addr)

static pmdval_t m_prot_sect_kernel = PMD_TYPE_SECT | PMD_SECT_AF | PMD_ATTRINDX(MT_NORMAL);

asmlinkage long (*origin_unlink_32)(const char __user *pathname) = NULL;
asmlinkage long (*origin_unlinkat_32)(int dfd, const char __user * pathname, int flag)= NULL;

static inline bool msm8994_needs_tlbi_wa(void)
{
	return false;
}