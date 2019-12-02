#include <types.h>
#include <vm.h>

#define PT_FREE 1
#define PT_CLEAN 2
#define PT_DIRTY 3
#define PT_FIXED 4


struct pagetable_entry {
	// struct lock *pte_lock;
    bool on_disk; //may need a disk variable for lseek
    paddr_t pfn;
    off_t pgentry_offset;
    pid_t pid;
    int state;
    int permission;
    struct pagetable_entry *next;
    struct pagetable_entry *prev;
};

struct pagetable {
    struct pagetable_entry *pagetable_arr;
    int fd;
}

void pagetable_init();

//int add_pagetable_entry(vaddr_t vaddr); //

//To be called by getppages
paadr_t pagetable_get(unsigned long npages);

//To be called by alloc_kpages();
paddr_t pagetable_alloc(unsigned npages);

void pagetable_evict(vaddr_t vaddr);

void pagetable_delete(vaddr_t vaddr);



//void delete_pagetable(struct pagetable) //may need delete pagetable entry from entry

/*
Things to implement
0. initializing page tables
    - n
1. vm_fault
    1. Page in memory but not tlb:
        1. tlb_write
    2. Page on disk but not memory:
        1. Allocate a place in physical memory to store the page
        2. Read the page from disk,
        3. Update the page table entry with the new virtual-to-physical address translation;
            1. How to do virtual to physical translation??
        4. Update the TLB to contain the new translation; and
        5. Resume execution of the user program.
    3. Eviction strategy policy - random eviction
2. Add paging
    - Data structure
    - API
        - Adding
        - Evicting/TLB shootdown/TLB shootdown_all
        - Copy
        - Writing to disk
            - swap allocation?? Where is this?
3. sbrk/malloc (30pts) -- trciky
    - Increase heap region
    - getppages

4. implement kfree
    - ??

4. If you evict a page from memory, you must invalidate the corresponding TLB entry, or to make sure that you never evict pages whose translations are present in any of the TLBs,

#define PADDR_TO_KVADDR(paddr) ((paddr)+MIPS_KSEG0) //physical to



*/
