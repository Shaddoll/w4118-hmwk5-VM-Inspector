


int expose_page_table(pid_t pid,
		  unsigned long fake_pgd,
		  unsigned long fake_pmds,
		  unsigned long page_table_addr,
		  unsigned long begin_vaddr,
		  unsigned long end_vaddr) {
	unsigned long temp_pgd = fake_pgd, temp_pmds = fake_pmds, temp_pte = page_table_addr, addr;
	int i,j,k,ret;

	struct task_struct *p;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *ptep, pte;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	//lock
	rcu_read_unlock();



	for (i = 0; i < PTRS_PER_PGD; i++) {

		pgd = pgd_offset(p->mm, i<<PGDIR_SHIFT);//
		if (pgd_none(*pgd) || pgd_bad(*pgd)) {
			temp_pgd += sizeof(unsigned long);
			continue;
		}

		ret = copy_to_user(temp_pgd, &temp_pmds, sizeof(unsigned long));
		if (ret != 0)
			return -EFAULT;
		temp_pgd += sizeof(unsigned long);

		for (j = 0; j < PTRS_PER_PMD; j++) {
			pmd = pmd_offset(pgd, (i<<PGDIR_SHIFT) + (j<<PMD_SHIFT));//
			if (pmd_none(*pmd) || pmd_bad(*pmd)) {
				temp_pmds += sizeof(unsigned long);
				continue;
			}

			ret = copy_to_user(temp_pmds, &temp_pte, sizeof(unsigned long));
			if (ret != 0)
				return -EFAULT;
			temp_pmds += sizeof(unsigned long);
			temp_pte += (PTRS_PER_PTE * sizeof(unsigned long));
		}
	}
	return 0;
}
