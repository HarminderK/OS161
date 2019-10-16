#include <types.h>
#include <syscall.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <vfs.h>
#include <limits.h>
#include <file.h>


int sys_open(const char *filename, int flags, mode_t mode, int *retval){
    int res;
    char * dest = kmalloc(sizeof(char *));
    res = copyinstr((const_userptr_t) filename, dest, NAME_MAX, NULL);
    if (res) {
        kfree(dest);
        return res;
    }
    res = file_open((char *)filename, flags, mode, retval);
    if (res) {
        kfree(dest);
        return res;
    }
    return 0;
}


