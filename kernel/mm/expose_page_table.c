#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <asm-generic/errno-base.h>

int do_expose_page_table(pid_t pid,
		unsigned long fake_pgd,
		unsigned long fake_pmds,
		unsigned long page_table_addr,
		unsigned long begin_vaddr,
		unsigned long end_vaddr) {
	unsigned long temp_pte = page_table_addr, i, j, count_pgd = 0, count_pmd = 0, size_pgd = 1, size_pmd = 1;
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

	pgd_kernel = kcalloc(size_pgd, sizeof(unsigned long), GFP_KERNEL);
	if (pgd_kernel == NULL)
		return -ENOMEM;
	
	pmd_kernel = kcalloc(size_pmd, sizeof(unsigned long), GFP_KERNEL);
	if (pmd_kernel == NULL)
		return -ENOMEM;

	if (p == NULL)
		return -EINVAL;

	mm = current->mm;
	read_lock(&tasklist_lock);
	spin_lock(&p->monitor_lock);
	lockid = p->monitor_pid;
	if (lockid != -1)
		p->monitor_pid = current->pid;
	spin_unlock(&p->monitor_lock);
	read_unlock(&tasklist_lock);
	if (lockid != -1)
		return -1;

	//lock page table, if p == current, should I grab this lock?
	spin_lock(&p->mm->page_table_lock);
	for (i = 0; i < PTRS_PER_PGD; i++) {

		pgd = pgd_offset(p->mm, i<<PGDIR_SHIFT);//
		if (pgd == NULL) {
			pgd_kernel[count_pgd++] = 0;
			continue;
		}

		pgd_kernel[count_pgd++] = fake_pmds + count_pmd * sizeof(unsigned long);

		for (j = 0; j < PTRS_PER_PMD; j++) {
			pmd = pmd_offset((pud_t *)pgd, (i<<PGDIR_SHIFT) + (j<<PMD_SHIFT));//
			if (pmd_none(*pmd) || pmd_bad(*pmd)) {
				pmd_kernel[count_pmd++] = 0;
				continue;
			}
			pmd_kernel[count_pmd++] = temp_pte;
			vma = find_vma(mm, temp_pte);
			down_write(&current->mm->mmap_sem);
			if (remap_pfn_range(vma, temp_pte, page_to_pfn(pmd_page(*pmd)), PAGE_SIZE, vma->vm_page_prot)) {
				up_write(&current->mm->mmap_sem);
				spin_unlock(&p->mm->page_table_lock);
				return -EAGAIN;
			}
			up_write(&current->mm->mmap_sem);
			temp_pte += (PTRS_PER_PTE * sizeof(unsigned long));
		}
	}
	spin_unlock(&p->mm->page_table_lock);
	//unlock page table
	ret = copy_to_user((void *)fake_pgd, (void *)pgd_kernel, count_pgd);
	if (ret != 0)
		return -EFAULT;
	ret = copy_to_user((void *)fake_pmds, (void *)pmd_kernel, count_pmd);
	if (ret != 0)
		return -EFAULT;
	for (i = 0; i < count_pgd; i++) {
		printk("Printing pgd %lu: %lu",i,pgd_kernel[i]);
	}
	for (i = 0; i < count_pmd; i++)
		printk("Printing pmd %lu: %lu",i,pmd_kernel[i]);
	return 0;
}




SYSCALL_DEFINE6(expose_page_table, pid_t, pid,
		unsigned long, fake_pgd,
		unsigned long, fake_pmds,
		unsigned long, page_table_addr,
		unsigned long, begin_vaddr,
		unsigned long, end_vaddr) {
	return do_expose_page_table(pid, fake_pgd, fake_pmds, page_table_addr, begin_vaddr, end_vaddr);
}
