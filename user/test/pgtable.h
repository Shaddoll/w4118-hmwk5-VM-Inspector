#ifndef _PGTABLE_H_
#define _PGTABLE_H_

#define __NR_expose_page_table	245
#define PGDIR_SHIFT		30
#define PMD_SHIFT		21
#define PAGE_SHIFT		12
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PMD_SIZE		(1UL << PMD_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE - 1))
#define PMD_MASK		(~(PMD_SIZE - 1))
#define PTRS_PER_PTE		512
#define PTRS_PER_PMD		512
#define PTRS_PER_PGD		512

#define pgd_index(addr)		(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))
#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
#define pte_index(addr)		(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

unsigned long get_phys_addr(unsigned long pte_entry)
{
	return (((1UL << 40) - 1) & pte_entry) >> 12 << 12;
}

int young_bit(unsigned long pte_entry)
{
	return ((1UL << 10) & pte_entry) ? 1 : 0;
}

int file_bit(unsigned long pte_entry)
{
	return ((1UL << 2) & pte_entry) ? 1 : 0;
}

int dirty_bit(unsigned long pte_entry)
{
	return ((1UL << 55) & pte_entry) ? 1 : 0;
}

int readonly_bit(unsigned long pte_entry)
{
	return ((1UL << 7) & pte_entry) ? 1 : 0;
}

int uxn_bit(unsigned long pte_entry)
{
	return ((1UL << 54) & pte_entry) ? 1 : 0;
}

#endif /* _PGTABLE_H_ */
