#include <types.h>
#include <syscall.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <vfs.h>
#include <limits.h>
#include <file.h>
#include <current.h>
#include <uio.h>
#include <kern/iovec.h>
#include <proc.h>
#include <kern/seek.h>
#include <stat.h>

int sys_open(const char *filename, int flags, mode_t mode, int *retval) {
    int res;
    char dest[PATH_MAX];
    res = copyinstr((const_userptr_t) filename, dest, NAME_MAX, NULL);
    if (res) {
        return res;
    }
    res = file_open(dest, flags, mode, retval);
    if (res) {
        return res;
    }
    return 0;
}

int sys_close(int fd) {
    struct file *file;
    int res;
    lock_acquire(curproc->p_filetable->ft_lock);
    file = curproc->p_filetable->files[fd];
    lock_release(curproc->p_filetable->ft_lock);
    if (file == NULL) {
        return EBADF;
    }

    res = filetable_remove(fd);
    return res;
}

int sys_read(int fd, void *buf, size_t buflen, int *retval) {
    struct iovec iovec;
    struct uio uio;
    struct file *file;
    int res;
    
    lock_acquire(curproc->p_filetable->ft_lock);
    file = curproc->p_filetable->files[fd];
    lock_release(curproc->p_filetable->ft_lock);
    if (file == NULL) {
        return EBADF;
    }

    lock_acquire(file->f_lock);
    uio_kinit(&iovec, &uio, buf, buflen, file->offset, UIO_READ);
    res = VOP_READ(file->f_vnode, &uio);
    if(res) {
        lock_release(file->f_lock);
        return res;
    }
    file->offset = uio.uio_offset;
    lock_release(file->f_lock);

    *retval = buflen - uio.uio_resid;
    return 0;
}

int sys_write(int fd, void *buf, size_t buflen, int *retval) {
    struct iovec iovec;
    struct uio uio;
    struct file *file;
    int res;
    struct filetable *filetable;
    lock_acquire(curproc->p_filetable->ft_lock);
    filetable = curproc->p_filetable;
    if(filetable == NULL) {
        kprintf("filetable is NULL\n");
    }
    file = curproc->p_filetable->files[fd];
    lock_release(curproc->p_filetable->ft_lock);
    if (file == NULL) {
        return EBADF;
    }

    lock_acquire(file->f_lock);
    uio_kinit(&iovec, &uio, buf, buflen, file->offset, UIO_WRITE);
    res = VOP_WRITE(file->f_vnode, &uio);
    if(res) {
        lock_release(file->f_lock);
        return res;
    }
    file->offset = uio.uio_offset;
    lock_release(file->f_lock);

    *retval = buflen - uio.uio_resid;
    return 0;
}

/**
 *  lseek alters the current seek position of the file handle filehandle, seeking to a new position based on pos and whence.

If whence is

    SEEK_SET, the new position is pos.
    SEEK_CUR, the new position is the current position plus pos.
    SEEK_END, the new position is the position of end-of-file plus pos.
    anything else, lseek fails. 
 * 
 *  On success, lseek returns the new position. On error, -1 is returned, and errno is set according to the error encountered. 
 * 
 * The following error codes should be returned under the conditions given. Other error codes may be returned for other cases not mentioned here.
EBADF 	fd is not a valid file handle.
ESPIPE		fd refers to an object which does not support seeking.
EINVAL		whence is invalid.
EINVAL		The resulting seek position would be negative.
 */ 
int
sys_lseek(int fd, off_t pos, int whence, off_t *retval)
{
    if(fd >= __OPEN_MAX || fd < 0) {
        return EBADF;
	}
    
    struct filetable *ft = curproc->p_filetable;
    struct file *file = ft->files[fd];
    lock_acquire(file->f_lock);

    if(file == NULL){
        return EBADF;
    }

    // res = VOP_ISSEEKABLE(file->f_vnode);
    // if(res){
    //     return ESPIPE;
    // }
    off_t newPos;
    int res;
    struct stat stat;
    
    switch (whence)
    {
        case SEEK_SET:
            newPos = pos;
            break;
        case SEEK_CUR:
            newPos = file->offset + pos;
            break;
        case SEEK_END:
            res = VOP_STAT(file->f_vnode, &stat);
            if(res){
                lock_release(file->f_lock);
                return res;
            }
            newPos = stat.st_size + pos;
            break;
        default:
            lock_release(file->f_lock);
            return EINVAL;
    }
    file->offset = newPos;
    *retval = newPos;

    if(newPos < 0){
        return EINVAL;
    }
    
    lock_release(file->f_lock);
    return 0;
}

/**
 *  The current directory of the current process is set to the directory named by pathname. 
 * 
 On success, chdir returns 0. On error, -1 is returned, and errno is set according to the error encountered.
Errors

The following error codes should be returned under the conditions given. Other error codes may be returned for other errors not mentioned here.
ENODEV 	The device prefix of pathname did not exist.
ENOTDIR		A non-final component of pathname was not a directory.
ENOTDIR		pathname did not refer to a directory.
ENOENT		pathname did not exist.
EIO		A hard I/O error occurred.
EFAULT		pathname was an invalid pointer.
 */
int
sys_chdir(const char *pathname)
{
    int res;
    char dest[PATH_MAX];
    res = copyinstr((const_userptr_t) pathname, dest, NAME_MAX, NULL);
    if (res) {
        return res;
    }
    res = vfs_chdir(dest);
    if (res) {
        return res;
    }
    return 0;
}

/**
 *  dup2 clones the file handle oldfd onto the file handle newfd. If newfd names an already-open file, that file is closed. 
 *  dup2 returns newfd. On error, -1 is returned, and errno is set according to the error encountered. 
Errors
The following error codes should be returned under the conditions given. Other error codes may be returned for other cases not mentioned here.
EBADF 	oldfd is not a valid file handle, or newfd is a value that cannot be a valid file handle.
EMFILE		The process's file table was full, or a process-specific limit on open files was reached.
ENFILE		The system's file table was full, if such a thing is possible, or a global limit on open files was reached.
*/
int
sys_dup2(int oldfd, int newfd, int *retval)
{
    if(oldfd >= __OPEN_MAX || oldfd < 0 || newfd >= __OPEN_MAX || newfd < 0) {
		return EBADF; // An index out of bound;
	}

    if(oldfd == newfd){
        return 0;
    }
    
    struct filetable *ft = curproc->p_filetable;

    struct file *file = ft->files[oldfd];

    if(ft->files[newfd] != NULL) {
        filetable_remove(newfd);
    }
    // struct file *dupFile;    

    // dupFile = kmalloc(sizeof(struct file));
    // dupFile->filename = file->filename;
    // dupFile->flag = file->flag;
    // dupFile->offset = file->offset;
    // dupFile->mode = file->mode;
    // dupFile->f_lock = lock_create("f_lock");
    // dupFile->f_vnode = file->f_vnode;
    lock_acquire(file->f_lock);
     if(file == NULL){
        return EBADF;
    }
    VOP_INCREF(file->f_vnode);
    lock_release(file->f_lock);
    
    lock_acquire(ft->ft_lock);
    ft->files[newfd] = file;
    lock_release(ft->ft_lock);

    *retval = newfd;
    return 0;
}

/**
 *  The name of the current directory is computed and stored in buf, an area of size buflen. The length of data actually stored, which must be non-negative, is returned. 
 *  On success, __getcwd returns the length of the data returned. On error, -1 is returned, and errno is set according to the error encountered.

Errors
The following error codes should be returned under the conditions given. Other error codes may be returned for other cases not mentioned here.
ENOENT 	A component of the pathname no longer exists.
EIO		A hard I/O error occurred.
EFAULT		buf points to an invalid address.
 */ 
int
sys___getcwd(char *buf, size_t buflen, int *retval)
{
    struct iovec iov;
    struct uio sys_uio;
    uio_kinit(&iov, &sys_uio, buf, buflen, 0, UIO_READ);

    int res = vfs_getcwd(&sys_uio);
    if(res){
        return res;
    }
    *retval = buflen - sys_uio.uio_resid;
    return 0;
}


