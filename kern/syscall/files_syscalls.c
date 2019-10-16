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


