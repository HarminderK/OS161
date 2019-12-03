#include <types.h>
#include <lib.h>
#include <pagetable.h>
#include <vm.h>
#include <synch.h>

struct pagetable_entry *pagetable;
// struct lock *pt_lock;
unsigned start_page, page_num;
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;

void pagetable_init(void)
{

    paddr_t end = ram_getsize();
    paddr_t start = ram_getfirstfree();
    unsigned npages, pagetable_size;

    npages = (end - start) / PAGE_SIZE;

    pagetable_size = npages * sizeof(struct pagetable_entry);
    pagetable_size = ROUNDUP(pagetable_size, PAGE_SIZE);

    pagetable = (struct pagetable_entry *)PADDR_TO_KVADDR(start);
    start += pagetable_size;

    if (start >= end)
    {
        panic("Run out of memory!\n");
    }

    start_page = start / PAGE_SIZE;
    page_num = (end / PAGE_SIZE) - start_page;

    struct pagetable_entry *tmp_ptentry;
    for (unsigned i = 0; i < page_num; i++)
    {
        struct pagetable_entry *cur = &pagetable[i];
        cur->pfn = (paddr_t)(i + start_page) * PAGE_SIZE;
        if (i == 0)
        {
            cur->prev = NULL;
        }
        else
        {
            if (i == page_num - 1)
            {
                cur->next = NULL;
            }
            cur->prev = tmp_ptentry;
            cur->prev->next = cur;
        }
        tmp_ptentry = cur;
        cur->state = PT_FREE;
    }
}

paddr_t pagetable_get(unsigned long npages)
{
    spinlock_acquire(&pt_lock);
    bool need_to_evict = true;
    struct pagetable_entry *base;
    struct pagetable_entry *cur = &pagetable[0];
    for (unsigned i = 0; i < page_num - npages; i++)
    {
        if (cur->state == PT_FREE)
        {
            need_to_evict = false;
            base = cur;
            for (unsigned j = 0; j < npages - 1; j++)
            {
                cur = cur->next;
                if (cur->next == NULL)
                {
                    spinlock_release(&pt_lock);
                    panic("Next page is NULL!\n");
                    return 0;
                }
                if (cur->next->state != PT_FREE)
                {
                    need_to_evict = true;
                    break;
                }
            }
            //found multiple contiguous free block
            if (!need_to_evict)
            {
                paddr_t pa = base->pfn;
                //mark the pages found to be Dirty
                for (unsigned k = 0; k < npages; k++) {
                    base->state = PT_DIRTY;
                    base = base->next;
                }
                spinlock_release(&pt_lock);
                return pa;
            }
        }
        cur = cur->next;
    }
    spinlock_release(&pt_lock);
    if (need_to_evict == true)
    {
        /* TODO: Uncomment after evict implemented */
        // base = pagetable_evict(npages);
        // return base->pfn;
        panic("Run out of memory, need to evict!\n");
        return 0;
    }
    return 0;
}

struct pagetable_entry *pagetable_evict(int npages)
{
    (void)npages;
    return NULL;
    // int page_num = (length - (length % PAGE_SIZE) + PAGE_SIZE) / PAGE_SIZE;
    // int evict = (((int)rand()) * page_num) % page_num;

    // if (npages > page_num)
    // {
    //     return null;
    // }

    // struct pagetable_entry *pg_entry = pagetable_entry[0];

    // if (pg_entry == null)
    // {
    //     return null;
    // }

    // spinlock_acquire(pt_lock);
    // int i = 0;
    // while (pg_entry->next != null && evict != i)
    // {
    //     pg_entry = pg_entry->next;
    //     i++;
    // }

    // i = 1;
    // struct pagetable_entry *start = pg_entry;
    // int res = pagetable_entry_to_disk(pg_entry);
    // start->state = PT_FREE;
    // while (pg_entry->next != null && i != npages)
    // {
    //     pg_entry = pg_entry->next;
    //     res = pagetable_entry_to_disk(pg_entry);
    //     pg_entry->state = PT_FREE;
    //     i++;
    // }
    // if (i != npages)
    // {
    //     while (start->prev != null && i != npages)
    //     {
    //         start = start->prev;
    //         res = pagetable_entry_to_disk(pg_entry);
    //         start->state = PT_FREE;
    //         i++;
    //     }
    // }
    // spinspinlock_release(pt_lock);
    // return start;
}

// int pagetable_entry_to_disk(struct pagetable_entry)
// {
//     if (!spinlock_do_i_hold(pt_lock))
//     {
//         return -1;
//     }
// }
