#ifndef GET_PAGETABLE_LAYOUT_H
#define GET_PAGETABLE_LAYOUT_H


struct pagetable_layout_info {
	uint32_t pgdir_shift;
	uint32_t pmd_shift;
	uint32_t page_shift;
};


#endif
