#ifndef _FIPC_FCNTL_H_
#define _FIPC_FCNTL_H_
#include <fcntl.h>

#define fipc_fcntl(fd, cmd, ...)                                                                                                \
        ((((fipc_fd *)&(fd))->mgmt.shm >= 0 && fcntl( ((fipc_fd *)&(fd))->mgmt.shm, cmd, # __VA_ARGS__) < 0) ? -1 :             \
        (((fipc_fd *)&(fd))->mgmt.wte >= 0 && fcntl( ((fipc_fd *)&(fd))->mgmt.wte, cmd, # __VA_ARGS__) < 0) ? -1 :              \
        (((fipc_fd *)&(fd))->mgmt.rde >= 0 && fcntl( ((fipc_fd *)&(fd))->mgmt.rde, cmd, # __VA_ARGS__) < 0) ? -1 :              \
        ((((fipc_fd *)&(fd))->mgmt.rde >=0 || ((fipc_fd *)&(fd))->mgmt.wte >=0 || ((fipc_fd *)&(fd))->mgmt.shm >=0) ? 0 : -1))

#endif
