#include <file.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <vnode.h>
#include <limits.h>
#include <synch.h>
#include <current.h>
#include <proc.h>
#include <lib.h>
#include <kern/errno.h>


 /* initialize filetable */ 
int filetable_init(struct filetable *ft) {
    
    // struct vnode *vn;
    // struct file *file_in;
    // struct file *file_out;
    // struct file *file_err;
    (void) ft;
    int fd = 0;
    int res;
    char filename[5];
    strcpy(filename, "con:");
    curproc->p_filetable = kmalloc(sizeof(struct filetable));
    int i = 0;
    
    for(i = 0; i < OPEN_MAX; i++){
        curproc->p_filetable->files[i] = NULL;
    }
    
    curproc->p_filetable->ft_lock = lock_create("filetable");

    /* enable consoles */
    // STDIN
    
    res = file_open(filename, O_RDONLY, 0, &fd);
    if(res) {
        return res;
    }
    //STDOUT
    strcpy(filename, "con:");
    res = file_open(filename, O_WRONLY, 0, &fd);
    if(res) {
        return res;
    }
    //STDERR
    strcpy(filename, "con:");
    res = file_open(filename, O_WRONLY, 0, &fd);
    if(res) {
        return res;
    }
    
    return 0;
}
 
/* removes a file from filetable */
int filetable_remove(int fd) {
	if(fd >= OPEN_MAX || fd < 0) {
		return -1; // Index out of bound;
	}
	lock_acquire(curproc->p_filetable->ft_lock);
	struct file *file = curproc->p_filetable->files[fd];
	
	if(file == NULL ){
		lock_release(curproc->p_filetable->ft_lock);
                        return -1;
	}
	curproc->p_filetable->files[fd] = NULL;
    lock_release(curproc->p_filetable->ft_lock);

    lock_acquire(file->f_lock);
    if(file->f_refcount > 1){
        file->f_refcount--;
        lock_release(file->f_lock);
    } else {
        lock_release(file->f_lock);
        file_destroy(file);
    }
	
 
return 0;
}


/* Destroy a filetable */
int filetable_destroy(struct filetable *ft) {
    (void) ft;
	lock_destroy(curproc->p_filetable->ft_lock);
	kfree(curproc->p_filetable);
	return 0;
}

/* add a file to filetable */
int filetable_add(struct file **file, int *fd) {
    lock_acquire(curproc->p_filetable->ft_lock);
	for( int i = 0; i < OPEN_MAX; i++) {
		if( curproc->p_filetable->files[i] == NULL) {
			curproc->p_filetable->files[i] = *file;
			*fd = i;
            lock_release(curproc->p_filetable->ft_lock);
			return 0;
		}
	}
    lock_release(curproc->p_filetable->ft_lock);
	return -1;
}
 
/* opens a file, allocating attributes in struct file. */
int file_open(char *filename, int flag, mode_t mode, int *fd) {
    struct vnode *vn;
    struct file *file;
    int res;
    // struct filetable *ft = curproc->p_filetable;
    
    res = vfs_open(filename, flag, mode, &vn); 
    if (res)  {
        return res;
        }
    
        file = kmalloc(sizeof(struct file));
        file->filename = filename;
        file->flag = flag;
        file->offset = 0;
        file->f_refcount = 1;
        file->mode = flag & O_ACCMODE;
        file->f_lock = lock_create("f_lock");
        file->f_vnode = vn;
    
    res = filetable_add(&file, fd);
    if(res) {
        return res;
    }

    return 0;
}

/* destroy a file struct */
int file_destroy(struct file *file){
    lock_acquire(file->f_lock);
    vfs_close(file->f_vnode);
    lock_release(file->f_lock);
	lock_destroy(file->f_lock);
	kfree(file);
    return 0;
}

/* copy current process's filetable */
int filetable_copy(struct filetable **new_ft){
    struct filetable *cur_ft = curproc-> p_filetable;

    *new_ft = kmalloc(sizeof(struct filetable));
    if(*new_ft == NULL) {
        return ENOMEM;
    }

    (*new_ft)->ft_lock = lock_create("ft");
    lock_acquire(cur_ft->ft_lock);
    for(int i = 0; i < OPEN_MAX; i ++) {
        if(cur_ft->files[i] != NULL) {
            lock_acquire(cur_ft->files[i]->f_lock);
            cur_ft->files[i]->f_refcount ++;
            (*new_ft)->files[i] = cur_ft->files[i];
            lock_release(cur_ft->files[i]->f_lock);
        } else {
            (*new_ft)->files[i] = NULL;
        }
        
    }
    lock_release(cur_ft->ft_lock);
    return 0;
}


