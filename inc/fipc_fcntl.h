#ifndef _FIPC_FCNTL_H_
#define _FIPC_FCNTL_H_
#include <fcntl.h>

int fipc_fcntl(int64_t fd, int cmd, int val);

#endif
