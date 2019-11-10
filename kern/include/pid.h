#ifndef _PID_H_
#define _PID_H_
 
#include <types.h>
#include <synch.h>
#include <limits.h>

 
/* struct pid for pid operations, 
* pid is the pid of current process and parent_pid is the pid of the parent process.
* exited represents if the process has exited or not. 
*/
struct pid {
    pid_t pid;
    // pid_t parent_pid;
    bool exited;
    int exit_status;
    struct lock *pid_lock;
    struct cv *pid_cv;
};
 
/*
	Init under run program
*/
    struct pid *procs[PID_MAX];
    struct lock *pm_lock;

/* initialize pid_manager */
int pid_manager_init(void);

/* create a new pid struct */
int pid_create( pid_t *new_pid);

/* destroy a pid struct */
int pid_destroy(pid_t pid);

/* get a pid struct */
struct pid* pid_get(pid_t pid);


#endif /* _PID_H_ */
