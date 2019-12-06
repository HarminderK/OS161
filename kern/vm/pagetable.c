#include <types.h>
#include <lib.h>
#include <pagetable.h>
#include <vm.h>
#include <synch.h>
#include <spinlock.h>
#include <bitmap.h>
#include <vnode.h>
#include <vfs.h>
#include <kern/iovec.h>
#include <uio.h>
#include <kern/fcntl.h>
#include <stat.h>

#define KVADDR_TO_PADDR(vaddr) ((vaddr)-0x80000000)

struct pagetable_entry *pagetable;
unsigned start_page, page_num;
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;
struct vnode *vn;
struct bitmap *bitmap;

/* Initilize the coremap, called by vm_bootstap */
void pagetable_init(void)
{
    //get the free sapce that is available
    paddr_t end = ram_getsize();
    paddr_t start = ram_getfirstfree();
    unsigned npages, pagetable_size;

    npages = (end - start) / PAGE_SIZE;

    pagetable_size = npages * sizeof(struct pagetable_entry);

    pagetable = (struct pagetable_entry *)PADDR_TO_KVADDR(start);
    start += pagetable_size;

    if (start >= end)
    {
        panic("Run out of memory!\n");
    }

    //Round the start page size up to align with PAGE_SIZE
    start = ROUNDUP(start, PAGE_SIZE);
    start_page = start / PAGE_SIZE;
    page_num = (end / PAGE_SIZE) - start_page;

    for (unsigned i = 0; i < page_num; i++)
    {
        struct pagetable_entry *cur = &pagetable[i];
        cur->pfn = (paddr_t)(i + start_page) * PAGE_SIZE;
        cur->state = PT_FREE;
        cur->size = 0;
        cur->start = NULL;
    }
}

/* Initialize vfs node for swapping */
void swap_bootstrap(void) {
    char diskpath[] = "lhd0raw:";
    struct stat disk_stat;
    // Open vnode
	int res = vfs_open(diskpath, O_RDWR, 0, &vn);
	if (res) {
		panic("" + res);
	}

    // Get swap stats
    res = VOP_STAT(vn, &disk_stat);
    if (res) {
		panic("" + res);
	}
    // Build a bitmap to keep track of swap writes
    bitmap = bitmap_create(disk_stat.st_size / PAGE_SIZE);
}

/* Find a free page or free pages, and return the physical address or the starting pages */
paddr_t pagetable_get(unsigned long npages)
{
    spinlock_acquire(&pt_lock);
    bool need_to_evict = true;
    int base;
    struct pagetable_entry *base_entry;
    for (unsigned i = 0; i < page_num - npages; i++)
    {
        struct pagetable_entry *cur = &pagetable[i];
        if (cur->state == PT_FREE)
        {
            need_to_evict = false;
            base = (int) i;
            i++;
            for (;i < (unsigned)base + npages; i++)
            {
                if (i == page_num)
                {
                    need_to_evict = true;
                    break;
                }
                if (pagetable[i].state != PT_FREE)
                {
                    need_to_evict = true;
                    break;
                }
            }
            //found multiple contiguous free block
            if (!need_to_evict)
            {
                base_entry = &pagetable[base];
                //mark the pages found to be Dirty
                for (unsigned k = (unsigned) base; k < (unsigned) base + npages; k++) {
                    struct pagetable_entry *tmp = &pagetable[k];
                    tmp->size = npages;
                    tmp->state = PT_DIRTY;
                    tmp->start = base_entry;
                }
                spinlock_release(&pt_lock);
                return base_entry->pfn;
            }
        }
    }
    spinlock_release(&pt_lock);
    if (need_to_evict == true)
    {
        base_entry = pagetable_evict(npages);
        return base_entry->pfn;
    }
    // Shouldn't reach here
    return 0;
}

/* Free a page or multiple pages */
int page_free(vaddr_t addr) {
    paddr_t pa = KVADDR_TO_PADDR(addr);
    struct pagetable_entry *entry;
    int i = 0;

    for (; i < (int)page_num; i++) {
        entry = &pagetable[i];
        if (entry->pfn == pa) {
            break;
        }
    }

    int end = i + entry->size;
    for (; i <  end; i++) {
        entry = &pagetable[i];
        entry->state = PT_FREE;
        entry->size = 0;
        entry->start = NULL;
    }
    return 0;
}

/* Evict Pagetable_entry from physical memory and write to memory */
struct pagetable_entry *pagetable_evict(int npages)
{
    int evict = (((int)random()) * page_num) % page_num;

    spinlock_acquire(&pt_lock);
    struct pagetable_entry *start = (&pagetable[evict])->start;

    // Go to the start of page table if there are more than 1 page allocated
    while(&pagetable[evict] != NULL && &pagetable[evict] != start){
        evict--;
    }
    int s = evict;
    int i = 0;

    // Set start to PT_DIRTY starting at the random evict index generated, while going forward
    while (&pagetable[evict] != NULL && i < npages)
    {
        int size = pagetable[evict].size;
        for(i = 0; i < size; i++){
            pagetable_entry_to_disk(&pagetable[evict]);
            pagetable[evict].state = PT_DIRTY;
            i++;
            evict++;
        }
    }
    // Set start to PT_DIRTY starting at the random evict index generated, while going backwards
    while (&pagetable[evict] == NULL && i < npages)
    {
        evict = s - 1;
        struct pagetable_entry *prev_start = (&pagetable[evict])->start;
        while(&pagetable[evict] != NULL && &pagetable[evict] != prev_start){
            pagetable_entry_to_disk(&pagetable[evict]);
            (&pagetable[evict])->state = PT_DIRTY;
            i++;
            evict--;
        }
        start = prev_start;
    }
    // Release and return the starting point of the free block
    spinlock_release(&pt_lock);
    return start;
}

// Write to pagetable entry to disk
int pagetable_entry_to_disk(struct pagetable_entry *pg_entry)
{
    if (!spinlock_do_i_hold(&pt_lock))
    {
        return -1;
    }

    unsigned int offset_index;
    // spinlock_acquire(&btm_lock);
	int res = bitmap_alloc(bitmap, &offset_index);
	// spinlock_release(&btm_lock);

    struct iovec iov;
    struct uio uio;
    pg_entry->pgentry_offset = (off_t) offset_index * PAGE_SIZE;

    uio_kinit(&iov, &uio, (void *)PADDR_TO_KVADDR(pg_entry->pfn), PAGE_SIZE, offset_index * PAGE_SIZE, UIO_WRITE);
    res = VOP_WRITE(vn, &uio);
	if(res) {
		return res;
	}
    return offset_index;
}
