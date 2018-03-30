#ifndef _FIPC_DEBUG_H
#define _FIPC_DEBUG_H

#include <stdint.h>

#ifndef NDEBUG

void get_dbg_info(int64_t fd, int64_t *mem_size, int64_t *read_size, int64_t *write_size);

#endif

#endif