#include <fcntl.h>
#include <fipc.h>
#include <internal.h>

int fipc_clear_fd_flag(int fd, int flag)
{
	int ret = fcntl(fd, F_GETFD);
	if (ret < 0)
		return ret;

	ret &= ~flag;
	ret = fcntl(fd, F_SETFD, ret);
	if (ret < 0)
		return ret;

	return 0;
}

#ifndef NDEBUG

void get_dbg_info(
	int64_t _fd, int64_t *mem_size, int64_t *read_size, int64_t *write_size)
{
	int64_t rs, ws;
	fipc_channel *channel = NULL;
	fipc_fd fd = { .raw = _fd };
	if (_fd < 0 || !(fd.mgmt.control & FIPC_FD_MASK))
		return;

	lock_fd(fd.mgmt.shm);

	channel = get_channel(fd.mgmt.shm);
	if (!channel)
		goto done;

	*mem_size = sizeof(fipc_channel);
	rs = atomic_set(&channel->read_size, 0);
	ws = atomic_set(&channel->write_size, 0);
	*read_size = rs;
	*write_size = ws;

done:
	unlock_fd(fd.mgmt.shm);
}

#endif
