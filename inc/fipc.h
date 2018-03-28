#ifndef _FIPC_H_
#define _FIPC_H_

#include <stdint.h>
#include <unistd.h>

int fipc(int64_t fipcfd[2], int robust);

int fipc2(int64_t fipcfd[2], int flags, int robust);

int fipc_close(int64_t fd);

int64_t fipc_dup(int64_t oldfd);

/* For any one who concerns: no dup2 or dup3! */


ssize_t fipc_read(int64_t fd, void *buf, size_t count);

ssize_t fipc_write(int64_t fd, void *buf, size_t count);

#endif
