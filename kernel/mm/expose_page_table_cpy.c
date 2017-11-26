#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <asm-generic/errno-base.h>

int get_pmd_start(unsigned long current_pgd, unsigned long base_pgd,
	unsigned long va_begin) {
	if (current_pgd != base_pgd)
		return 0;
	return pmd_index(va_begin);
}
int get_pmd_end(unsigned long current_pgd, unsigned long last_pgd,
	unsigned long va_end) {
	if (current_pgd != last_pgd)
		return PTRS_PER_PMD;
	return pmd_index(va_end) + 1;
}
int do_expose_page_table(pid_t pid,
		unsigned long fake_pgd,
		unsigned long fake_pmds,
		unsigned long page_table_addr,
		unsigned long va_begin,
		unsigned long va_end) {
	unsigned long temp_pte = page_table_addr;
	unsigned long count_pgd = 0, count_pmd = 0;
	unsigned size_pgd, size_pmd, i, j;
	int ret,lockid;

	unsigned long *pgd_kernel;
	unsigned long *pmd_kernel;

	struct task_struct *p;
	pgd_t *pgd;
	pmd_t *pmd;
	struct vm_area_struct *vma;
	struct mm_struct *mm;

	if (pid == -1)
		p = current;
	else {
		rcu_read_lock();
		p = find_task_by_vpid(pid);
		rcu_read_unlock();
	}

	size_pgd = PTRS_PER_PGD;
	size_pmd = (pgd_index(va_end) - pgd_index(va_begin) + 1) *
		   PTRS_PER_PMD;

	pgd_kernel = kcalloc(size_pgd, sizeof(unsigned long), GFP_KERNEL);
	if (pgd_kernel == NULL)
		return -ENOMEM;
	pmd_kernel = kcalloc(size_pmd, sizeof(unsigned long), GFP_KERNEL);
	if (pmd_kernel == NULL) {
		kfree(pgd_kernel);
		return -ENOMEM;
	}
	if (p == NULL) {
		kfree(pmd_kernel);
		kfree(pgd_kernel);
		return -EINVAL;
	}

	mm = current->mm;
	read_lock(&tasklist_lock);
	spin_lock(&p->monitor_lock);
	lockid = p->monitor_pid;
	if (lockid == -1)
		p->monitor_pid = current->pid;
	p->monitor_va_begin = va_begin & PAGE_MASK;
	p->monitor_va_end = va_end & PAGE_MASK;
	p->monitor_va_page_table = page_table_addr;
	spin_unlock(&p->monitor_lock);
	if (lockid != -1) {
		kfree(pmd_kernel);
		kfree(pgd_kernel);
		return -1;
	}

	//lock page table, if p == current, should I grab this lock?
	if (p != current)
		spin_lock(&p->mm->page_table_lock);
	for (i = pgd_index(va_begin); i <= pgd_index(va_end); i++) {

		pgd = pgd_offset(p->mm, i<<PGDIR_SHIFT);
		pgd_kernel[i] = fake_pmds +
			count_pgd * PTRS_PER_PMD * sizeof(unsigned long);
		count_pgd++;
		if (*pgd == 0) {
			count_pmd += 
				get_pmd_end(i, pgd_index(va_end), va_end) -
				get_pmd_start(i, pgd_index(va_begin), va_begin);
			continue;
		}

		for (j = get_pmd_start(i, pgd_index(va_begin), va_begin);
			j < get_pmd_end(i, pgd_index(va_end), va_end);
			j++) {
			pmd = pmd_offset((pud_t *)pgd,
				(i<<PGDIR_SHIFT) + (j<<PMD_SHIFT));//

			temp_pte = page_table_addr +
				   count_pmd * PTRS_PER_PTE *
				   sizeof(unsigned long);
			pmd_kernel[count_pgd * PTRS_PER_PMD + j] = temp_pte;
			count_pmd++;

			if (*pmd == 0) {//check if should use if (pmd_none(*pmd) || pmd_bad(*pmd)) {
				continue;
			}

			//temp_pte = page_table_addr +
			//	(i * PTRS_PER_PGD * PTRS_PER_PMD +
			//	j * PTRS_PER_PMD) * sizeof(unsigned long);//
			//pmd_kernel[i * PTRS_PER_PMD + j] = temp_pte;

			vma = find_vma(mm, temp_pte);
			printk("i: %d, j: %d, pmd: %#x, temp_pte: %#x, *pmd: %#x\n", i, j, pmd, temp_pte, *pmd);
			if (*pmd) {
				int k;
				unsigned long *ptr = (unsigned long*)pmd_page_vaddr(*pmd);
				for (k = 0; k < 512; ++k) {
					if (ptr[k])
						printk("pte[%d]: %#x\n", k, ptr[k]);
				}
			}
			down_write(&current->mm->mmap_sem);
			if (p != current)
				down_write(&p->mm->mmap_sem);
			if (remap_pfn_range(vma, temp_pte,
				page_to_pfn(pmd_page(*pmd)),
				PAGE_SIZE, vma->vm_page_prot)) {
				if (p != current)
					up_write(&p->mm->mmap_sem);
				up_write(&current->mm->mmap_sem);
				if (p != current)
					spin_unlock(&p->mm->page_table_lock);
				kfree(pgd_kernel);
				kfree(pmd_kernel);
				read_unlock(&tasklist_lock);
				return -EAGAIN;
			}
			if (p != current)
				up_write(&p->mm->mmap_sem);
			up_write(&current->mm->mmap_sem);
		}
	}

	if (p != current)
		spin_unlock(&p->mm->page_table_lock); //unlock page table
	read_unlock(&tasklist_lock);
	ret = copy_to_user((void *)fake_pgd, (void *)pgd_kernel,
		PTRS_PER_PGD * sizeof(unsigned long));
	if (ret != 0) {
		kfree(pgd_kernel);
		kfree(pmd_kernel);
		return -EFAULT;
	}
	ret = copy_to_user((void *)fake_pmds,
		(void *)pmd_kernel,
		count_pgd * PTRS_PER_PMD * sizeof(unsigned long));
	if (ret != 0) {
		kfree(pgd_kernel);
		kfree(pmd_kernel);
		return -EFAULT;
	}
	return 0;
}




SYSCALL_DEFINE6(expose_page_table,
		pid_t, pid,
		unsigned long, fake_pgd,
		unsigned long, fake_pmds,
		unsigned long, page_table_addr,
		unsigned long, begin_vaddr,
		unsigned long, end_vaddr)
{
	return do_expose_page_table(pid,
				    fake_pgd,
				    fake_pmds,
				    page_table_addr,
				    begin_vaddr,
				    end_vaddr);
}