#include "get_pagetable_layout.h"
#include <asm/page.h>
#include <asm/pgtable-3level-hwdef.h>


SYSCALL_DEFINE2(get_pagetable_layout,
		struct pagetable_layout_info *, pgtbl_info
		int, size)
{
	struct pagetable_layout_info pli;
	pli.pgdir_shift = PGDIR_SHIFT;
	pli.pmd_shift = PMD_SHIFT;
	pli.page_shift = PAGE_SHIFT;
	
	if (copy_to_user() != 0) {
	}
}

