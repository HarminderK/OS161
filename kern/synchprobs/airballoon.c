/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>


#define N_LORD_FLOWERKILLER 8
#define NROPES 16
static int ropes_left = NROPES;

/* struct representing ropes 
** ropes is an array, the array index correspond to a certain rope, and the value represent if the rope is loose or not. 1-tight, 0-loose
** count is the count of the rope left, for end condition of each thread.
*/
struct rope {
	volatile int ropes[NROPES];
	volatile int count;
};

/* thread_status for the main thread to wait until all other threads done */
struct thread_status {
	volatile bool dan_done;
	volatile bool mar_done;
	volatile bool fkiller_done;
	volatile bool ballon_done;
	volatile int fkiller_count;
};

/* sturct initialization */
static struct rope *rope;
static struct thread_status *thread_status;

/* stake and hook maps */
volatile int stakes_map[NROPES]; //map stake to rope index
volatile int hook_map[NROPES]; //map hook to rope index

/* Synchronization primitives */
 struct semaphore *sem;
 struct lock *lk;


/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	kprintf("Dandelion thread starting\n");
	int index = random() % 16;
	P(sem);
	while (rope->count > 0) {
		kprintf("count is:%d  ", rope->count);
		while ((hook_map[index] == -1) || (rope->ropes[hook_map[index]] == 0)) {
			index = random() % 16;
		}
		rope->ropes[hook_map[index]] = 0;
		rope->count --;
		kprintf("Dandelion severed rope %d\n", hook_map[index]);
		hook_map[index] = -1;
		V(sem);
		thread_yield();
		P(sem);
	}
	kprintf("Dandelion thread done\n");
	V(sem);
	lock_acquire(lk);
	thread_status->dan_done = true;
	lock_release(lk);
	return;
}

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	int index = random() % 16;
	kprintf("Marigold thread starting\n");
	P(sem);
	while(rope->count > 0) {
		kprintf("count is:%d  ", rope->count);
		while ((stakes_map[index] == -1) || (rope->ropes[stakes_map[index]] == 0)) {
			index = random() % 16;
		}
		rope->ropes[stakes_map[index]] = 0;
		rope->count --;
		kprintf("Marigold severed rope %d from stake %d\n", stakes_map[index], index);
		stakes_map[index] = -1;
		V(sem);
		thread_yield();
		P(sem);
	}
	kprintf("Marigold thread done\n");
	V(sem);
	lock_acquire(lk);
	thread_status->mar_done = true;
	lock_release(lk);
	return;
}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Lord FlowerKiller thread starting\n");
	int f_index = random() % 16;
	P(sem);
	while(rope->count > 1) {
		while ((stakes_map[f_index] == -1) || (rope->ropes[stakes_map[f_index]] == 0)) {
			f_index = random() % 16;
		}
		int stake1 = f_index;
		while ((f_index == stake1) || ((stakes_map[f_index] == -1) || (rope->ropes[stakes_map[f_index]] == 0)) ) {
			f_index = random() % 16;
		}
		int stake2 = f_index;
		int rope1 = stakes_map[stake1];
		int rope2 = stakes_map[stake2];
		stakes_map[stake1] = rope2;
		stakes_map[stake2] = rope1;
		kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", rope1, stake1, stake2);
		kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", rope2, stake2, stake1);
		V(sem);
		thread_yield();
		P(sem);
	}
	kprintf("Lord FlowerKiller thread done\n");
	V(sem);
	lock_acquire(lk);
	thread_status->fkiller_count --;
	if(thread_status->fkiller_count == 0) {
		thread_status->fkiller_done = true;
	}
	lock_release(lk);
	return;
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");

	P(sem);
	while(rope->count > 0) {
		V(sem);
		thread_yield();
		P(sem);
	}
	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	V(sem);
	lock_acquire(lk);
	thread_status->ballon_done = true;
	lock_release(lk);
	kprintf("Balloon thread done\n");
	return;
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

	int err = 0, i;

	(void)nargs;
	(void)args;
	(void)ropes_left;

	/*initialize lock primitives*/
	sem = sem_create("sem", 1);
	if (sem == NULL) {
		panic("sem_create failed\n");
	}
	lk = lock_create("lk");
	if (lk == NULL) {
		panic("lock_create failed\n");
	}

	/*initialize thread_status struct*/
	thread_status = kmalloc(sizeof(*thread_status));
	thread_status->dan_done = false;
	thread_status->mar_done = false;
	thread_status->fkiller_done = false;
	thread_status->ballon_done = false;
	thread_status->fkiller_count = N_LORD_FLOWERKILLER;


	/*initialize rope stuct*/
	rope = kmalloc(sizeof(*rope));
	rope->count = NROPES;
	for (int i = 0; i < NROPES; i++) {
		rope->ropes[i] = 1;
	}

	/* initialize stake_mapping and hook_mapping */
	for(int i = 0; i < NROPES; i++) {
		hook_map[i] = i;
		stakes_map[i] = i;	
	}

	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;

	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	lock_acquire(lk);
	while(( (!thread_status->dan_done) || (!thread_status->mar_done) ) || 
	( (!thread_status->fkiller_done) || (!thread_status->ballon_done) )) {
		lock_release(lk);
		thread_yield();
		lock_acquire(lk);
	}
	lock_release(lk);
	
	kfree(rope);
	kfree(thread_status);
	sem_destroy(sem);
	lock_destroy(lk);

	kprintf("Main thread done\n");
	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
	return 0;
}
