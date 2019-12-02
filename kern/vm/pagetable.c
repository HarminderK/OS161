#include <types.h>
#include <lib>
#include <pagetable.h>
#include <vm.h>
#include <spinlock.h>

struct pagetable_entry *pagetable;
struct spinlock *pt_lock;
unsigned start_page, page_num;

void pagetable_init()
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
    for (int i = 0; i < page_num; i++)
    {
        struct pagetable_entry *cur = &pagetable[i];
        cur->pfn = (paddr_t) (i + start_page)*PAGE_SIZE;
        if (i == 1)
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
        tmp_ptentry = &pagetable[i];
        pt[i].state = PT_FREE;
    }
    spinlock_init(pt_lock);
}

paadr_t pagetable_get(unsigned long npages)
{
    spinlock_acquire(pt_lock);
    for (int i = 0; i < page_num; i++)
    {
        struct pagetable_entry *cur = &pagetable[i];
        if (cur->state == PT_FREE)
        {
            for (int j = i; j < i + npages; j++)
            {
                cur->state = PT_DIRTY;
                cur = cur->next;
            }
            break;
        }
    }
    spinlock_release(pt_lock);
}

paddr_t pagetable_alloc(unsigned npages)
{
}

struct *pagetable_entry pagetable_evict(int npages)
{
    int page_num = (length - (length % PAGE_SIZE) + PAGE_SIZE) / PAGE_SIZE;
    int evict = (((int)rand()) * page_num) % page_num;

    if (npages > page_num)
    {
        return null;
    }

    struct pagetable_entry *pg_entry = pagetable_entry[0];

    if (pg_entry == null)
    {
        return null;
    }

    spinlock_acquire(pt_lock);
    int i = 0;
    while (pg_entry->next != null && evict != i)
    {
        pg_entry = pg_entry->next;
        i++;
    }

    i = 1;
    struct pagetable_entry *start = pg_entry;
    int res = pagetable_entry_to_disk(pg_entry);
    start->state = PT_FREE;
    while (pg_entry->next != null && i != npages)
    {
        pg_entry = pg_entry->next;
        res = pagetable_entry_to_disk(pg_entry);
        pg_entry->state = PT_FREE;
        i++;
    }
    if (i != npages)
    {
        while (start->prev != null && i != npages)
        {
            start = start->prev;
            res = pagetable_entry_to_disk(pg_entry);
            start->state = PT_FREE;
            i++;
        }
    }
    spinlock_release(pt_lock);
    return start;
}

int pagetable_entry_to_disk(struck pagetable_entry)
{
    if (!spinlock_do_i_hold(pt_lock))
    {
        return -1;
    }
}
