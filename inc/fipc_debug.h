#ifndef _FIPC_DEBUG_H
#define _FIPC_DEBUG_H

#include <stdint.h>

#ifdef DEBUG

typedef struct _fipc_debug_info
{
        int64_t read_contents;
        int64_t write_contents;
        int64_t syscalls;
        int64_t backlog_used;
} fipc_debug_info;

void fipc_get_dbg_info(int64_t fd, fipc_debug_info *info);

void fipc_clear_dbg_info(int64_t fd);

#endif

#endif