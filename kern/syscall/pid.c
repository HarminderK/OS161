#include <pid.h>
#include <current.h>
#include <proc.h>
#include <limits.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <kern/wait.h>

// struct pid *procs[PID_MAX];
struct lock *pm_lock;
// struct cv *pid_cv;

/* Initialize the global pid_manager, called in boot() */
int pid_manager_init(void)
{
    pm_lock = lock_create("pm_lock");
    // pid_cv = cv_create("pidcv");
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
    // pid->pid_lock = lock_create("pidlock");
    // if (pid->pid_lock == NULL)
    // {
    //     kfree(pid);
    //     lock_release(pm_lock);
    //     return ENOMEM;
    // }
    pid->pid_cv = cv_create("pidcv");
    if (pid->pid_cv == NULL)
    {
        // lock_destroy(pid->pid_lock);
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
    // lock_acquire(pm_lock);
    if (procs[pid] != NULL)
    {
        struct pid *t_pid = procs[pid];
        // lock_destroy(t_pid->pid_lock);
        cv_destroy(t_pid->pid_cv);
        kfree(t_pid);
    }
    procs[pid] = NULL;
    // lock_release(pm_lock);
    return 0;
}

/* return the pid struct given a pid*/
struct pid *pid_get(pid_t pid)
{
    struct pid *cur_pid;
    // lock_acquire(pm_lock);
    cur_pid = procs[(int)pid];
    // lock_release(pm_lock);
    return cur_pid;
}

/* wait fot a pid to exit */
int pid_wait(pid_t pid, int *retval){
    lock_acquire(pm_lock);
    struct pid *cur_pid = pid_get(pid);

    if (cur_pid == NULL)
    {
        lock_release(pm_lock);
        return ESRCH;
    }
    /* Prevent from waiting for itself */
    if((curproc->p_pid == pid) || (pid_get(curproc->p_pid)->ppid == pid) ) {
        lock_release(pm_lock);
        return ECHILD;
    }
    bool isChild = false;

    /* menu process */
    if (pid == 2)
    {
        isChild = true;
    }

    if(cur_pid->ppid == curproc->p_pid) {
        isChild = true;
    }

    if (!isChild)
    {
        lock_release(pm_lock);
        return ECHILD;
    }

    /* wait for the pid to exit, if it has exited, return the status and pid */
    if (cur_pid->exited == false)
    {
        cv_wait(cur_pid->pid_cv, pm_lock);
    }

    *retval = cur_pid->exit_status;
    lock_release(pm_lock);
    return 0;
}

/* exit the current process */
void pid_exit(int exitcode){
    lock_acquire(pm_lock);
    struct pid *pid = pid_get(curproc->p_pid);

    // pid_t t_pid = -1;
    exitcode = _MKWAIT_EXIT(exitcode);
    pid->exited = true;
    pid->exit_status = exitcode;
    // lock_acquire(pm_lock);
    for (int i = 2; i < PID_MAX; i++)
    {
        struct pid *tmp = pid_get(i);
        if (tmp != NULL)
        {
            if(tmp->ppid == curproc->p_pid && tmp->exited == true) {
                pid_destroy(tmp->pid);
            }
        }
    }
    
    if (pid_get(pid->ppid) != NULL && (pid_get(pid->ppid)->exited == false))
    {
        cv_signal(pid->pid_cv, pm_lock);
        lock_release(pm_lock);
        sys_exit_helper(curproc);
    }
     else 
    {
        lock_release(pm_lock);
        pid_destroy(curproc->p_pid);
        sys_exit_helper(curproc);
    }
}
