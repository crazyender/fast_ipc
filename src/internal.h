#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <stdint.h>

#define FIPC_CHANNEL_SIZE (2 * 1024 * 1024)
#define FIPC_BLOCK_SIZE (32 * 1024)
#define FIPC_BLOCK_NUMBER ((FIPC_CHANNEL_SIZE) / (FIPC_BLOCK_SIZE))
#define DEFAULT_PIPE_SIZE (64 * 1024)

#define FIPC_FD_MASK 	0x4
#define FIPC_FD_PIPE 	0x1
#define FIPC_FD_EVENT	0x2

#define atomic_add_and_fetch(x, y) __sync_add_and_fetch((x), (y))
#define atomic_fetch_and_add(x, y) __sync_fetch_and_add((x), (y))
#define atomic_get(x) atomic_add_and_fetch(x, 0)
#define atomic_set(x, y) __sync_lock_test_and_set((x), (y))
#define atomic_inc(x) atomic_add_and_fetch(x, 1)
#define atomic_dec(x) atomic_add_and_fetch(x, -1)

typedef union _fipc_fd
{
	struct
	{
		int control : 4;
		int shm : 20;
		int rde : 20;
		int wte : 20;
	} mgmt;
	int64_t raw;
}fipc_fd;

typedef struct _fipc_block
{
	int64_t available;
	int64_t amount;
	int64_t dirty;
	const char buf[FIPC_BLOCK_SIZE];
}fipc_block;

typedef struct _fipc_channel
{
	int64_t pipe_bytes;
	int64_t rd_idx;
	int64_t wt_idx;
	fipc_block blocks[FIPC_BLOCK_NUMBER];
}fipc_channel;

int64_t fipc_box(int shm, int read_event, int write_event);

int fipc_unbox_shm(int64_t fd);

int fipc_unbox_rde(int64_t fd);

int fipc_unbox_wte(int64_t fd);

#endif
