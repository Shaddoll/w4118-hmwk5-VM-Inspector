#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include "pgtable.h"

void print_page_table(unsigned long fake_pgd,
		      unsigned long va_begin,
		      unsigned long va_end,
		      int verbose)
{
	const unsigned long top_addr = 0x8000000000;
	unsigned long page = va_begin & PAGE_MASK;
	unsigned long end_page = (va_end + PAGE_SIZE - 1) & PAGE_MASK;
	unsigned long *pgd_ptr = (unsigned long *)fake_pgd;
	unsigned long *pmd_ptr, *pte_ptr;
	unsigned long pgd_entry, pmd_entry, pte_entry;

	end_page = end_page < top_addr ? end_page : top_addr;
	//pgd_entry = pgd_ptr[pgd_index(page)];
	//pmd_ptr = (unsigned long *)pgd_entry;
	//pmd_entry = pmd_ptr[pmd_index(page)];
	//printf("%lx\n", pmd_entry);
	while (page < end_page) {
		pgd_entry = pgd_ptr[pgd_index(page)];
		if (pgd_entry == 0) {
			if (verbose)
				printf("0x%lx 0x%lx %d %d %d %d %d\n",
					page, 0L, 0, 0, 0, 0, 0);
			page = (page + PGDIR_SIZE) & PGDIR_MASK;
			continue;
		}
		pmd_ptr = (unsigned long *)pgd_entry;
		pmd_entry = pmd_ptr[pmd_index(page)];
		if (pmd_entry == 0) {
			if (verbose)
				printf("0x%lx 0x%lx %d %d %d %d %d\n",
					page, 0L, 0, 0, 0, 0, 0);
			page = (page + PMD_SIZE) & PMD_MASK;
			continue;
		}
		pte_ptr = (unsigned long *)pmd_entry;
		pte_entry = pte_ptr[pte_index(page)];
		if (pte_entry || verbose) 
			printf("0x%lx 0x%lx %d %d %d %d %d\n", page,
				get_phys_addr(pte_entry), young_bit(pte_entry),
				file_bit(pte_entry), dirty_bit(pte_entry),
				readonly_bit(pte_entry), uxn_bit(pte_entry));
		page += PAGE_SIZE;
	}
}

int main(int argc, char* argv[])
{
	pid_t pid;
	unsigned long va_begin, va_end, fake_pgd, fake_pmds, page_table_addr;
	int verbose = 0, track = 0;
	int ret, pgd_size, pmd_size, pte_size;

	if (argc == 4) {
		if (sscanf(argv[1], "%d", &pid) < 0 ||
		    sscanf(argv[2], "%lx", &va_begin) < 0 ||
		    sscanf(argv[3], "%lx", &va_end) < 0)
			goto parse_error;
	} else if (argc == 5) {
		if (strcmp(argv[1], "-v") == 0)
			verbose = 1;
		else if (strcmp(argv[1], "-track") == 0)
			track = 1;
		else
			goto parse_error;
		if (sscanf(argv[2], "%d", &pid) < 0 ||
		    sscanf(argv[3], "%lx", &va_begin) < 0 ||
		    sscanf(argv[4], "%lx", &va_end) < 0)
			goto parse_error;
	} else if (argc == 6) {
		if (strcmp(argv[1], "-v") == 0 &&
		    strcmp(argv[2], "-track") == 0)
			verbose = track = 1;
		else if (strcmp(argv[1], "-track") == 0 &&
			 strcmp(argv[2], "-v") == 0)
			verbose = track = 1;
		else
			goto parse_error;
		if (sscanf(argv[3], "%d", &pid) < 0 ||
		    sscanf(argv[4], "%lx", &va_begin) < 0 ||
		    sscanf(argv[5], "%lx", &va_end) < 0)
			goto parse_error;
	} else {
parse_error:
		printf("usage: ./<exec> [-v] [-track] pid va_begin va_end");
		return 1;
	}

	pgd_size = PTRS_PER_PGD * sizeof(unsigned long);
	pmd_size = (pgd_index(va_end) - pgd_index(va_begin) + 1) *
		   PTRS_PER_PMD * sizeof(unsigned long);
	//pmd_size = PTRS_PER_PGD * PTRS_PER_PMD * sizeof(unsigned long);
	pte_size = pgd_index(va_end) == pgd_index(va_begin) ?
		   (pmd_index(va_end) - pmd_index(va_begin) + 1) *
		   PTRS_PER_PTE * sizeof(unsigned long) :
		   ((PTRS_PER_PMD - pmd_index(va_begin)) +
		   (pgd_index(va_end) - pgd_index(va_begin)) * PTRS_PER_PMD +
		   (pmd_index(va_end) + 1)) * PTRS_PER_PTE * sizeof(unsigned long);
	//pte_size = ((va_end >> PAGE_SHIFT) - (va_begin >> PAGE_SHIFT) + 1) *
	//	   PTRS_PER_PTE * sizeof (unsigned long);
	//pte_size = PTRS_PER_PTE * PTRS_PER_PMD * PTRS_PER_PGD * sizeof(unsigned long);

	fake_pgd = (unsigned long) mmap(NULL, pgd_size, PROT_WRITE | PROT_READ,
					MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	fake_pmds = (unsigned long) mmap(NULL, pmd_size, PROT_WRITE | PROT_READ,
					 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	page_table_addr = (unsigned long) mmap(NULL, pte_size, PROT_READ,
					       MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	ret = syscall(__NR_expose_page_table,
		      pid,fake_pgd, fake_pmds, page_table_addr,
		      va_begin, va_end);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return 1;
	}
	printf("%lx, %lx\n", page_table_addr, pte_size);
	print_page_table(fake_pgd,
			 va_begin,
			 va_end,
			 verbose);
	while (track) {
		sleep(5);
		print_page_table(fake_pgd,
			 	 va_begin,
			 	 va_end,
				 verbose);
	}
	return 0;
}
