#include <stdio.h>
#include "pgtable.h"

int main(int argc, char* argv[])
{
	pid_t pid;
	unsigned long va_begin, va_end, fake_pgd, fake_pmds, page_table_addr;
	bool verbose = false, track = false;
	int ret;

	if (argc == 4) {
		if (sscanf(argv[1], "%d", &pid) < 0 ||
		    sscanf(argv[2], "%x", &va_begin) < 0 ||
		    sscanf(argv[3], "%x", &va_end) < 0)
			goto parse_error;
	} else if (argc == 5) {
		if (strcmp(argv[1], "-v") == 0)
			verbose = true;
		else if (strcmp(argv[1], "-track") == 0)
			track = true;
		else
			goto parse_error;
		if (sscanf(argv[2], "%d", &pid) < 0 ||
		    sscanf(argv[3], "%x", &va_begin) < 0
		    sscanf(argv[4], "%x", &va_end) < 0)
			goto parse_error;
	} else if (argc == 6) {
		if (strcmp(argv[1], "-v") == 0 &&
		    strcmp(argv[2], "-track") == 0)
			verbose = track = true;
		else if (strcmp(argv[1], "-track") == 0 &&
			 strcmp(argv[2], "-v") == 0)
			verbose = track = true;
		else
			goto parse_error;
		if (sscanf(argv[3], "%d", &pid) < 0 ||
		    sscanf(argv[4], "%x", &va_begin) < 0 ||
		    sscanf(argv[5], "%x", &va_end) < 0)
			goto parse_error;
	} else {
parse_error:
		printf("usage: ./<exec> [-v] [-track] pid va_begin va_end");
		return 1;
	}

	fake_pgd = mmap(NULL, ,
			PROT_WRITE | PROT_READ,
		        MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	fake_pmds = mmap(NULL, ,
			 PROT_WRITE | PROT_READ,
			 MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	page_table_addr = mmap(NULL, ,
			       PROT_READ,
			       MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	ret = syscall(__NR_expose_page_table,
		      pid,fake_pgd, fake_pmds, page_table_addr,
		      va_begin, va_end);
	if (ret < 0) {
		fprintf(stderr, "error: %s", strerror(errno));
		return 1;
	}
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

void print_page_table(unsigned long fake_pgd,
		      unsigned long va_begin,
		      unsigned_long va_end,
		      bool verbose)
{
	unsigned long page = va_begin & (~(PAGE_SIZE - 1));
	unsigned long end_page = (va_end + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));
	unsigned long *pgd_ptr = (unsigned long *)fake_pgd;
	unsigned long *pmd_ptr, *pte_ptr;
	unsigned long pgd_entry, pmd_entry, pte_entry;

	for (; page < end_page; page += PAGE_SIZE) {
		pgd_entry = pgd_ptr[pgd_index(page)];
		if (pgd_entry == NULL) {
			if (verbose)
				printf("%x %x %d %d %d %d %d\n",
					page, 0, 0, 0, 0, 0, 0);
			continue;
		}
		pmd_ptr = (unsigned long *)pgd_entry;
		pmd_entry = pmd_ptr[pmd_index(page)];
		if (pmd_entry == NULL) {
			if (verbose)
				printf("%x %x %d %d %d %d %d\n",
					page, 0, 0, 0, 0, 0, 0);
			continue;
		}
		pte_ptr = (unsigned long *)pmd_entry;
		pte_entry = pte_ptr[pte_index(page)];
		printf("%x %x %d %d %d %d %d\n", page,
			get_phys_addr(pte_entry), young_bit(pte_entry),
			file_bit(pte_entry), dirty_bit(pte_entry),
			readonly_bit(pte_entry), uxn_bit(pte_entry));
	}
}
