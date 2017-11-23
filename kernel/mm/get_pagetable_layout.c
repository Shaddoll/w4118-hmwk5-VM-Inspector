#include "get_pagetable_layout.h"
#include <asm/page.h>
#include <asm/pgtable-3level-hwdef.h>


SYSCALL_DEFINE2(get_pagetable_layout,
		struct pagetable_layout_info __user *, pgtbl_info
		int, size)
{
	struct pagetable_layout_info k_pgtbl_info;
	k_pgtbl_info.pgdir_shift = PGDIR_SHIFT;
	k_pgtbl_info.pmd_shift = PMD_SHIFT;
	k_pgtbl_info.page_shift = PAGE_SHIFT;

	if (pgtbl_info == NULL)
		return -EINVAL;

	if (size != sizeof(struct pagetable_layout_info))
		return -EINVAL;

	if (copy_to_user(pgtbl_info, k_pgtbl_info, size) != 0)
		return -EFAULT;

	return 0;
}

