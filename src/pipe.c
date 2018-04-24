#define _GNU_SOURCE
#include <fcntl.h>
#include <fipc.h>
#include <internal.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

static char pesudo_pipe_buffer[4096];
static const int default_pipe_buffer_size = (64 * 1024);

static int configure_pipe_size(int fd)
{
#ifdef F_SETPIPE_SZ
	int pipe_size = (int)sysconf(_SC_PAGESIZE);
	if (pipe_size < 0)
	{
		return pipe_size;
	}

	int ret = fcntl(fd, F_SETPIPE_SZ, pipe_size);
	if (ret < 0)
	{
		return ret;
	}
#else

#endif
	return 0;
}

int get_pipe_size(int fd)
{
#ifdef F_GETPIPE_SZ
	int pipe_size = fcntl(fd, F_GETPIPE_SZ);
	if (pipe_size < 0)
	{
		return pipe_size;
	}
	return pipe_size;
#else
	return default_pipe_buffer_size;
#endif
}

int pipe_open(fipc_fd fds[2])
{
	int ret;
	int pipefd[2] = {-1, -1};

	/* using pipe */
	ret = pipe(pipefd);
	if (ret < 0)
		goto fail;

	// change pipe buffer size
	if (configure_pipe_size(pipefd[0]) < 0)
		goto fail;

	if (configure_pipe_size(pipefd[1]) < 0)
		goto fail;

	ret = fipc_clear_fd_flag(pipefd[0], FD_CLOEXEC);
	if (ret < 0)
		goto fail;

	ret = fipc_clear_fd_flag(pipefd[1], FD_CLOEXEC);
	if (ret < 0)
		goto fail;

	ret = fipc_clear_fd_flag(pipefd[0], O_NONBLOCK);
	if (ret < 0)
		goto fail;

	ret = fipc_clear_fd_flag(pipefd[1], O_NONBLOCK);
	if (ret < 0)
		goto fail;

	fds[0].mgmt.control = FIPC_FD_PIPE;
	fds[0].mgmt.rde = pipefd[0];
	fds[0].mgmt.wte = -1;
	fds[1].mgmt.control = FIPC_FD_PIPE;
	fds[1].mgmt.rde = -1;
	fds[1].mgmt.wte = pipefd[1];
	return 0;

fail:
	if (pipefd[0] >= 0)
		close(pipefd[0]);

	if (pipefd[1] >= 0)
		close(pipefd[1]);

	return -1;
}

int pipe_close(fipc_fd fd)
{
	int ret = 0;
	if (fd.mgmt.rde >= 0)
		ret = (close(fd.mgmt.rde) < 0);

	if (fd.mgmt.wte >= 0)
		ret |= (close(fd.mgmt.wte) < 0);

	if (ret)
		return -1;
	else
		return 0;
}

int pipe_wait_rde(fipc_fd fd, fipc_channel *channel)
{
	int ret = channel->flags;
	int timeout = 0;
	struct pollfd pollfd;
	int errorevent = POLLHUP | POLLERR | POLLNVAL;
	fipc_block *block = &channel->blocks[(channel->rd_idx - 1) % FIPC_BLOCK_NUMBER];
	if (atomic_get(&block->status) != 0)
		return 1;

#ifdef POLLRDHUP
	errorevent |= POLLRDHUP;
#endif
	if (ret < 0)
		return ret;

	if (ret & O_NONBLOCK)
		timeout = 0;
	else
		timeout = -1;

	pollfd.fd = fd.mgmt.rde;
	pollfd.events = POLLIN | POLLERR;
	pollfd.revents = 0;
	ret = poll(&pollfd, 1, timeout);
	if (ret <= 0)
		return ret;

	if (pollfd.revents & errorevent)
		return -1;

	return 1;
}

int pipe_notify_wte(fipc_fd fd, fipc_channel *channel)
{
	int read_size = channel->pipe_size / FIPC_BLOCK_NUMBER;
	int size_left = read_size;
	int ret = 0;
	fipc_block *block = &channel->blocks[(channel->rd_idx - 1) % FIPC_BLOCK_NUMBER];

	while (size_left)
	{
		ret = read(fd.mgmt.rde, pesudo_pipe_buffer, size_left);
		if (ret <= 0)
			return ret;
		size_left -= ret;
	}
	atomic_set(&block->status, 0);
	return read_size;
}

int pipe_wait_wte(fipc_fd fd, fipc_channel *channel)
{
	int ret = channel->flags;
	int timeout = 0;
	struct pollfd pollfd;
	int errorevent = POLLHUP | POLLERR | POLLNVAL;
	fipc_block *block = &channel->blocks[(channel->wt_idx - 1) % FIPC_BLOCK_NUMBER];
	if (atomic_get(&block->status) == 0)
		return 1;

#ifdef POLLRDHUP
	errorevent |= POLLRDHUP;
#endif

	if (ret < 0)
		return ret;

	if (ret & O_NONBLOCK)
		timeout = 0;
	else
		timeout = -1;

	pollfd.fd = fd.mgmt.wte;
	pollfd.events = POLLOUT | POLLERR;
	pollfd.revents = 0;
	ret = poll(&pollfd, 1, timeout);
	if (ret <= 0)
		return ret;

	if (pollfd.revents & errorevent)
		return -1;

	return 1;
}

int pipe_notify_rde(fipc_fd fd, fipc_channel *channel)
{
	int write_size = channel->pipe_size / FIPC_BLOCK_NUMBER;
	int size_left = write_size;
	int ret = 0;
	fipc_block *block = &channel->blocks[(channel->wt_idx - 1) % FIPC_BLOCK_NUMBER];

	while (size_left)
	{
		ret = write(fd.mgmt.wte, pesudo_pipe_buffer, size_left);
		if (ret <= 0)
			return ret;
		size_left -= ret;
	}
	atomic_set(&block->status, 1);
	return write_size;
}

int pipe_poll(struct fipc_pollfd *fds, nfds_t nfds, int timeout)
{
	// FIXME
	return -1;
}

fipc_op pipe_op = {.open = pipe_open,
				   .close = pipe_close,
				   .wait_rde = pipe_wait_rde,
				   .wait_wte = pipe_wait_wte,
				   .notify_rde = pipe_notify_rde,
				   .notify_wte = pipe_notify_wte,
				   .poll = pipe_poll};

#ifndef __linux__
fipc_op eventfd_op = {.open = pipe_open,
					  .close = pipe_close,
					  .wait_rde = pipe_wait_rde,
					  .wait_wte = pipe_wait_wte,
					  .notify_rde = pipe_notify_rde,
					  .notify_wte = pipe_notify_wte,
					  .poll = pipe_poll};
#endif
