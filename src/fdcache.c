#include <internal.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_CACHE_FD 10 * 1024
static void *fds[MAX_CACHE_FD] = {NULL};
static pthread_rwlock_t locks[MAX_CACHE_FD];

void fd_cache_init()
{
	int i = 0;
	for (i = 0; i < MAX_CACHE_FD; i++)
	{
		pthread_rwlock_init(&locks[i], NULL);
	}
}

void lock_fd_read(int fd)
{
	if (fd < 0 || fd >= MAX_CACHE_FD)
		return;

	pthread_rwlock_rdlock(&locks[fd]);
}

void lock_fd_write(int fd)
{
	if (fd < 0 || fd >= MAX_CACHE_FD)
		return;

	pthread_rwlock_wrlock(&locks[fd]);
}

void unlock_fd(int fd)
{
	if (fd < 0 || fd >= MAX_CACHE_FD)
		return;

	pthread_rwlock_unlock(&locks[fd]);
}

fipc_channel *get_channel(int fd)
{
	if (fd < 0 || fd >= MAX_CACHE_FD)
		return NULL;

	// fast path
	if (fds[fd] != NULL)
		return fds[fd];

	// slow path
	if (fcntl(fd, F_GETFL) < 0)
		return NULL;

	void *addr = mmap(NULL, sizeof(fipc_channel), PROT_READ | PROT_WRITE,
					  MAP_SHARED, fd, 0);
	if (addr == (void *)MAP_FAILED)
		return NULL;

	fds[fd] = addr;
	return fds[fd];
}

void clear_channel(int fd)
{
	if (fd < 0 || fd >= MAX_CACHE_FD)
		return;

	if (fds[fd] == NULL)
		return;

	munmap((void *)fds[fd], sizeof(fipc_channel));
	fds[fd] = NULL;
}
