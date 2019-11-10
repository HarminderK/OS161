#include <pid.h>
#include <current.h>
#include <limits.h>
#include <kern/errno.h>
#include <lib.h>

struct pid *procs[PID_MAX];
struct lock *pm_lock;

/* Initialize the global pid_manager, called in boot() */
int pid_manager_init(void)
{
    pm_lock = lock_create("pm_lock");
    for (int i = 0; i < PID_MAX; i++)
    {
        procs[i] = NULL;
    }
    return 0;
}

/* create a new pid in pid_manager */
int pid_create(pid_t *new_pid)
{
    lock_acquire(pm_lock);
    pid_t pid_index = -1;
    for (int i = 1; i < PID_MAX; i++)
    {
        if (procs[i] == NULL)
        {
            pid_index = i;
            break;
        }
    }
    if (pid_index == -1)
    {
        lock_release(pm_lock);
        return ENPROC;
    }
    struct pid *pid = kmalloc(sizeof(struct pid));
    if (pid == NULL)
    {
        lock_release(pm_lock);
        return ENOMEM;
    }
    pid->pid = pid_index;
    // pid->parent_pid = parent_pid;
    pid->exited = false;
    pid->pid_lock = lock_create("pidlock");
    if (pid->pid_lock == NULL)
    {
        kfree(pid);
        lock_release(pm_lock);
        return ENOMEM;
    }
    pid->pid_cv = cv_create("pidcv");
    if (pid->pid_cv == NULL)
    {
        lock_destroy(pid->pid_lock);
        kfree(pid);
        lock_release(pm_lock);
        return ENOMEM;
    }
    procs[pid_index] = pid;
    *new_pid = pid_index;
    lock_release(pm_lock);
    return 0;
}

/* destroy a pid in the pid_manager */
int pid_destroy(pid_t pid)
{
    lock_acquire(pm_lock);
    if (procs[pid] != NULL)
    {
        struct pid *t_pid = procs[pid];
        lock_destroy(t_pid->pid_lock);
        cv_destroy(t_pid->pid_cv);
        kfree(t_pid);
    }
    procs[pid] = NULL;
    lock_release(pm_lock);
    return 0;
}

/* return the pid struct given a pid*/
struct pid *pid_get(pid_t pid)
{
    struct pid *cur_pid;
    lock_acquire(pm_lock);
    cur_pid = procs[(int)pid];
    lock_release(pm_lock);
    return cur_pid;
}
