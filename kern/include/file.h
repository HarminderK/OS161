#ifndef _FILE_H_
#define _FILE_H_
 
#include <types.h>
#include <synch.h>
#include <vnode.h>
#include <limits.h>
 
/* The file struct, containing File name, Open flags,
 * File offset, A lock for synchronization, pointer to vnode
 */
 
struct file {
    char *filename;
    int flag;
    volatile __off_t offset;
    int f_refcount;
    int mode;
    struct lock *f_lock;
    struct vnode *f_vnode;
};
 
/* File table struct, an array of files
 */
 
struct filetable {
	struct file *files[OPEN_MAX]; // OPEN_MAX is defined in limits.h -- 128
	struct lock *ft_lock;
};
 
int filetable_init(struct filetable *ft);
int filetable_destroy(struct filetable *ft);
int filetable_add( struct file **file, int *fd);
int filetable_remove( int fd);
int file_open(char *filename, int flags, mode_t mode, int *fd);
int file_destroy(struct file *file);

 
#endif /* _FILE_H_ */



