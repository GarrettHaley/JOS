// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *current = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    if(!(uvpt[(uint32_t)current / PGSIZE] & PTE_COW))
        panic("PGFAULT: access %e", err);
    if (!(err & FEC_WR))
         panic("PGFAULT: access %e", err);
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's currentess.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
    r = sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P);
    if (r < 0)
        panic("SYS_PAGE_ALLOC: %e", r);

    memmove(PFTEMP, (void*)((uint32_t)current / PGSIZE * PGSIZE), PGSIZE);
    r = sys_page_map(0, PFTEMP, 0, (void*)((uint32_t)current / PGSIZE * PGSIZE), PTE_P|PTE_U|PTE_W);
    if (r < 0)
        panic("SYS_PAGE_MAP: %e", r);

    r = sys_page_unmap(0, PFTEMP);
    if ( r < 0)
         panic("SYS_PAGE_UNMAP: %e", r);
}

//
// Map our virtual page pn (currentess pn*PGSIZE) into the target envid
// at the same virtual currentess.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
    if (uvpt[pn] & PTE_SHARE) {
        r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), uvpt[pn] & PTE_SYSCALL);
        if (r < 0) {
            cprintf("SYS_PGMAP FAIL: %e\n", r);
            return r;
        }
    else
        return 0;
  }

    if (uvpt[pn] & PTE_W || uvpt[pn] & PTE_COW) {
        r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), PTE_P | PTE_U | PTE_COW);
        if (r < 0)
            return r;

        r =  sys_page_map(envid, (void*)(pn * PGSIZE), 0, (void*)(pn * PGSIZE), PTE_P | PTE_U | PTE_COW);
        if (r < 0)
            return r;

    }
    else {
        r =  sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), uvpt[pn] & PTE_SYSCALL);
        if (r < 0) {
            cprintf("SYS_PAGE_MAP FAIL: %e\n", r);
            return r;
        }
    }
    return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our currentess space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
    // LAB 4: Your code here.
    envid_t kiddie;
    set_pgfault_handler(pgfault);
    kiddie = sys_exofork();
    extern void _pgfault_upcall();

    if(kiddie == 0){
        thisenv = &envs[ENVX(sys_getenvid())];
        return kiddie;
    }
    if(kiddie < 0)
        panic("FORKING CHILD WAS UNSUCCESSFUL!");

    for(uintptr_t current = 0; current < USTACKTOP; current += PGSIZE){
        if((uvpd[PDX(current)] & PTE_P) &&  (uvpt[PGNUM(current)] & PTE_P) && (uvpt[PGNUM(current)] & PTE_U))
                duppage(kiddie,PGNUM(current));
    }

    if(sys_page_alloc(kiddie,(void*)(UXSTACKTOP-PGSIZE),PTE_P|PTE_U|PTE_W)){
        panic("ALLOC EXCEPTION STACK FAILED!");
    }

    sys_env_set_pgfault_upcall(kiddie,_pgfault_upcall);
    if(sys_env_set_status(kiddie,ENV_RUNNABLE))
        panic("ENV SET STATUS FAILED!");

    return kiddie;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
