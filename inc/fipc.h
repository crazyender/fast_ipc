#ifndef _FIPC_H_
#define _FIPC_H_

#include <stdint.h>
#include <unistd.h>

typedef enum _fipc_type
{
        FIPC_FD_PIPE = 0x1,
        FIPC_FD_EVENT = 0x2,
        FIPC_FD_SPIN = 0x3,
        FIPC_FD_MASK= 0xff,
}fipc_type;


typedef union _fipc_fd
{
	struct _mgmt
	{
		short rde;
		short wte;
		short shm;
		short control;
	} mgmt;
	int64_t raw;
}fipc_fd;

void fipc_init();

int fipc(int64_t fipcfd[2], fipc_type type);

int fipc2(int64_t fipcfd[2], int flags, fipc_type type);

int fipc_close(int64_t fd);

int64_t fipc_dup(int64_t oldfd);

/* For any one who concerns: no dup2 or dup3! */


ssize_t fipc_read(int64_t fd, void *buf, size_t count);

ssize_t fipc_write(int64_t fd, void *buf, size_t count);

#endif
