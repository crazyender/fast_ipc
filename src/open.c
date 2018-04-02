#include <errno.h>
#include <fcntl.h>
#include <fipc.h>
#include <fipc_fcntl.h>
#include <internal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

static int64_t shm_name_index = -1;

int fipc(int64_t fipcfd[2], fipc_type type)
{
	int64_t name_index = atomic_inc(&shm_name_index);
	char name[64] = { 0 };
	int shm_fd = -1;
	int shm_fd2 = -1;
	fipc_fd events_fd[2] = { -1, -1 };
	int ret = -1;
	int i = 0;

	if (fipcfd == NULL) {
		errno = EINVAL;
		goto fail;
	}

	snprintf(name, 64, "/shm_%d_%ld.fipc", getpid(), name_index);
	shm_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (shm_fd < 0)
		goto fail;

	// here is the tricky to avoid share memory leak
	ret = shm_unlink(name);
	if (ret < 0)
		goto fail;

	ret = fipc_clear_fd_flag(shm_fd, FD_CLOEXEC);
	if (ret < 0)
		goto fail;

	ret = get_op(type)->open(events_fd);
	if (ret < 0)
		goto fail;

	ret = ftruncate(shm_fd, sizeof(fipc_channel));
	if (ret < 0)
		goto fail;

	shm_fd2 = dup(shm_fd);
	if (shm_fd2 < 0)
		goto fail;

	events_fd[0].mgmt.shm = shm_fd;
	events_fd[1].mgmt.shm = shm_fd2;
	fipcfd[0] = events_fd[0].raw;
	fipcfd[1] = events_fd[1].raw;
	return 0;

fail:
	if (shm_fd >= 0)
		close(shm_fd);

	if (shm_fd2 >= 0)
		close(shm_fd2);

	for (i = 0; i < 2; i++) {
		if (events_fd[i].raw < 0)
			continue;

		get_op(type)->close(events_fd[i]);
	}

	return -1;
}

int fipc2(int64_t fipcfd[2], int flags, fipc_type type)
{
	int ret = -1;
	int i = 0;

	ret = fipc(fipcfd, type);
	if (ret < 0)
		return ret;

	// a dirty hack
	if ((flags & O_NONBLOCK) && (type == FIPC_FD_SPIN)) {
		((fipc_fd *)&fipcfd[0])->mgmt.rde = -2;
		((fipc_fd *)&fipcfd[0])->mgmt.wte = -2;
		((fipc_fd *)&fipcfd[1])->mgmt.rde = -2;
		((fipc_fd *)&fipcfd[1])->mgmt.wte = -2;
	}

	for (i = 0; i < 2; i++) {
		ret = fipc_fcntl(fipcfd[i], F_SETFD, flags);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int fipc_close(int64_t _fd)
{
	fipc_fd fd = { .raw = _fd };
	int ret = -1;

	if (_fd < 0) {
		errno = EINVAL;
		return -1;
	}

	if (!(fd.mgmt.control & FIPC_FD_MASK))
		return close((int)_fd);

	close(fd.mgmt.shm);
	ret = get_op(fd.mgmt.control & FIPC_FD_MASK)->close(fd);
	lock_fd(fd.mgmt.shm);
	clear_channel(fd.mgmt.shm);
	// after closing, map shm fd will fail
	unlock_fd(fd.mgmt.shm);
	return ret;
}

int64_t fipc_dup(int64_t fd)
{
	fipc_fd oldfd = { .raw = fd };
	fipc_fd newfd = { .raw = -1 };

	if (fd < 0) {
		errno = EINVAL;
		return -1;
	}

	if (!(oldfd.mgmt.control & FIPC_FD_MASK)) {
		return dup((int)fd);
	}

	oldfd.raw = fd;
	newfd.mgmt.control = oldfd.mgmt.control;
	newfd.mgmt.shm = dup(oldfd.mgmt.shm);
	if (newfd.mgmt.shm < 0)
		goto fail;

	newfd.mgmt.wte = dup(oldfd.mgmt.wte);
	if (newfd.mgmt.wte < 0)
		goto fail;

	newfd.mgmt.rde = dup(oldfd.mgmt.rde);
	if (newfd.mgmt.rde < 0)
		goto fail;

	return newfd.raw;

fail:
	if (newfd.mgmt.shm >= 0)
		close(newfd.mgmt.shm);

	if (newfd.mgmt.wte >= 0)
		close(newfd.mgmt.wte);

	if (newfd.mgmt.rde >= 0)
		close(newfd.mgmt.rde);

	return -1;
}

void fipc_init()
{
	fd_cache_init();
}
