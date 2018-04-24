#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <fipc.h>
#include <fipc_poll.h>
#include <stdint.h>

#define FIPC_CHANNEL_SIZE (2 * 1024 * 1024)
#define FIPC_BLOCK_SIZE (32 * 1024)
#define FIPC_BLOCK_NUMBER ((FIPC_CHANNEL_SIZE) / (FIPC_BLOCK_SIZE))
#define DEFAULT_PIPE_SIZE (64 * 1024)

#define atomic_add_and_fetch(x, y) __sync_add_and_fetch((x), (y))
#define atomic_fetch_and_add(x, y) __sync_fetch_and_add((x), (y))
#define atomic_get(x) *((volatile int64_t *)x)
#define atomic_set(x, y) (*x = y)
#define atomic_inc(x) atomic_add_and_fetch(x, 1)
#define atomic_dec(x) atomic_add_and_fetch(x, -1)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef struct _fipc_block
{
	char buf[FIPC_BLOCK_SIZE];
	int64_t status;
	int64_t amount;
	int64_t offset;
} fipc_block;

typedef struct _fipc_channel
{
	fipc_block blocks[FIPC_BLOCK_NUMBER];
	fipc_block backlog;
#ifndef NDEBUG
	int64_t read_size;
	int64_t write_size;
#endif
	int64_t rd_idx;
	int64_t wt_idx;
	int64_t pipe_size;
	int64_t flags;
} fipc_channel;

typedef struct _fipc_op
{
	int (*open)(fipc_fd fds[2]);
	int (*close)(fipc_fd fd);
	int (*wait_rde)(fipc_fd fd, fipc_channel *block);
	int (*notify_wte)(fipc_fd fd, fipc_channel *block);
	int (*wait_wte)(fipc_fd fd, fipc_channel *block);
	int (*notify_rde)(fipc_fd fd, fipc_channel *block);
	int (*poll)(struct fipc_pollfd *fds, nfds_t nfds, int timeout);
} fipc_op;

int fipc_clear_fd_flag(int fd, int flag);

void fd_cache_init();

void lock_fd_read(int fd);

void lock_fd_write(int fd);

void unlock_fd(int fd);

fipc_op *get_op(fipc_type type);

fipc_channel *get_channel(int fd);

void clear_channel(int fd);

int get_pipe_size(int fd);

#endif
