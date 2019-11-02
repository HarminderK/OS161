#include <pid.h>
#include <current.h>
#include <limits.h>
#include <kern/errno.h>
#include <lib.h>
 
struct pid_manager *pid_manager;
 
/* Initialize the global pid_manager, called in boot() */
int pid_manager_init(void) {
    pid_manager = kmalloc(sizeof(struct pid_manager));
	if(pid_manager == NULL) {
		return ENOMEM;
	}
    pid_manager->pm_lock = lock_create("pm_lock");
    for (int i = 0; i < PID_MAX; i++){
        pid_manager->procs[i] = NULL;
    }
	return 0;
} 
 
/* create a new pid in pid_manager */
int pid_create( pid_t *new_pid) {
    struct pid_manager *t_pid_manager = pid_manager;
    lock_acquire(t_pid_manager->pm_lock);
    pid_t pid_index = -1;
    for( int i = 1; i < PID_MAX; i++){
        if(t_pid_manager->procs[i] == NULL){
            pid_index = i;
            break;
        }
    }
    if(pid_index  == -1){
        lock_release(t_pid_manager->pm_lock);
        return ENPROC;
    }
    struct pid *pid = kmalloc(sizeof(struct pid));
    pid->pid = pid_index;
    // pid->parent_pid = parent_pid;
    pid->exited = false;
    pid->pid_lock = lock_create("pidlock");
    pid->pid_cv = cv_create("pidcv");
    t_pid_manager->procs[pid_index] = pid;
	*new_pid =pid_index;
    lock_release(t_pid_manager->pm_lock);
    return 0;
}
 
/* destroy a pid in the pid_manager */
int pid_destroy(pid_t pid) {
    struct pid_manager *t_pid_manager = pid_manager;
    lock_acquire(t_pid_manager->pm_lock);
    if(t_pid_manager->procs[pid] != NULL){
        struct pid *t_pid = t_pid_manager->procs[pid];
        lock_destroy(t_pid->pid_lock);
        kfree(t_pid);
    }
    t_pid_manager->procs[pid] = NULL;
    lock_release(t_pid_manager->pm_lock);
	return 0;
}

/* return the pid struct given a pid*/
struct pid* pid_get(pid_t pid) {
    return pid_manager->procs[(int)pid];
}
