#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <asm-generic/errno-base.h>

int expose_page_table(pid_t pid,
		  unsigned long fake_pgd,
		  unsigned long fake_pmds,
		  unsigned long page_table_addr,
		  unsigned long begin_vaddr,
		  unsigned long end_vaddr) {
	unsigned long temp_pgd = fake_pgd, temp_pmds = fake_pmds, temp_pte = page_table_addr, addr, phys;
	int i,j,k,ret,lockid;

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

	if (p == NULL)
		return -EINVAL;

	mm = current->mm;
	//vma = find_vma(mm, temp_pte);
	read_lock(&tasklist_lock);
	spin_lock(&p->monitor_lock);
	lockid = p->monitor_pid;
	if (lockid != -1)
		p->monitor_pid = current->pid;
	spin_unlock(&p->monitor_lock);
	read_unlock(&tasklist_lock);
	if (lockid != -1)
		return -1;

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
			phys = page_to_pfn(pmd_page(*pmd));
			ret = copy_to_user(temp_pmds, &temp_pte, sizeof(unsigned long));
			if (ret != 0)
				return -EFAULT;
			temp_pmds += sizeof(unsigned long);
			vma = find_vma(mm, temp_pte);
			if (remap_pfn_range(vma, temp_pte, phys, PAGE_SIZE, vma->vm_page_prot))
				return -EAGAIN;
			temp_pte += (PTRS_PER_PTE * sizeof(unsigned long));
		}
	}
	return 0;
}
