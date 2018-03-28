#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <fipc.h>
#include <internal.h>
#include <stdio.h>
#include <fipc_fcntl.h>

static int64_t shm_name_index = -1;
static char pesudo_pipe_buffer[4096];

static int fipc_clear_fd_flag(int fd, int flag)
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

static int fipc_open_events_fd(fipc_fd fds[2], int robust)
{
	int pipefd[2] = {-1, -1};
	int rdfd = -1;
	int wtfd = -1;
	int ret;

	if (robust) {
		/* using pipe */
		ret = pipe(pipefd);
		if (ret < 0)
			goto fail;

		// FIXME
		// change pipe buffer size

		ret = fipc_clear_fd_flag(pipefd[0], FD_CLOEXEC);
		if (ret < 0)
			goto fail;

		ret = fipc_clear_fd_flag(pipefd[1], FD_CLOEXEC);
		if (ret < 0)
			goto fail;

		fds[0].mgmt.control = FIPC_FD_PIPE;
		fds[0].mgmt.rde = pipefd[0];
		fds[0].mgmt.wte = -1;
		fds[1].mgmt.control = FIPC_FD_PIPE;
		fds[1].mgmt.rde = -1;
		fds[1].mgmt.wte = pipefd[1];
		return 0;

	} else {
		/* using eventfd */
		rdfd = eventfd(0, EFD_SEMAPHORE);
		if (rdfd < 0)
			goto fail;

		wtfd = eventfd(FIPC_BLOCK_NUMBER, EFD_SEMAPHORE);
		if (wtfd < 0)
			goto fail;

		fds[0].mgmt.control = FIPC_FD_EVENT;
		fds[0].mgmt.rde = rdfd;
		fds[0].mgmt.wte = wtfd;
		fds[1].mgmt.control = FIPC_FD_EVENT;
		fds[1].mgmt.rde = dup(rdfd);
		fds[1].mgmt.wte = dup(wtfd);
		if (fds[1].mgmt.rde < 0 || fds[1].mgmt.wte < 0)
			goto fail;

		return 0;

	}

	return 0;

fail:
	if (pipefd[0] >= 0)
		close(pipefd[0]);

	if (pipefd[1] >= 0)
		close(pipefd[1]);

	if (rdfd >= 0)
		close(rdfd);

	if (wtfd >= 0)
		close(wtfd);

	return -1;

}

static void fipc_close_events_fd(fipc_fd fd)
{
	if (fd.mgmt.rde >= 0)
		close(fd.mgmt.rde);

	if (fd.mgmt.wte >= 0)
		close(fd.mgmt.wte);
}

static int fipc_get_write_token(int64_t _fd, /* option */ int bytes)
{
	fipc_fd fd;
	int64_t event = 1;
	fd.raw = _fd;


	if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_PIPE)
		return write(fd.mgmt.wte, pesudo_pipe_buffer, bytes);
	else if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_EVENT)
		return write(fd.mgmt.wte, &event, sizeof(event));
	else
		return -1;
}

static int fipc_get_read_token(int64_t _fd, /* option */ int bytes)
{
	fipc_fd fd;
	int64_t event = 1;
	fd.raw = _fd;


	if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_PIPE)
		return read(fd.mgmt.rde, pesudo_pipe_buffer, bytes);
	else if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_EVENT)
		return read(fd.mgmt.rde, &event, sizeof(event));
	else
		return -1;

}


int fipc(int64_t fipcfd[2], int robust)
{
	int64_t name_index = atomic_inc(&shm_name_index);
	char name[64] = {0};
	int shm_fd = -1;
	int shm_fd2 = -1;
	fipc_fd events_fd[2] = {-1, -1};
	int ret = -1;
	int i = 0;

	if (fipcfd == NULL) {
		errno = EINVAL;
		goto fail;
	}

	snprintf(name, 64, "/shm_%d_%ld.fipc", getpid(), name_index);
	shm_fd = shm_open(name,
			  O_RDWR | O_CREAT | O_EXCL,
			  S_IRUSR | S_IWUSR);
	if (shm_fd < 0)
		goto fail;

	ret = fipc_clear_fd_flag(shm_fd, FD_CLOEXEC);
	if (ret < 0)
		goto fail;

	ret = fipc_open_events_fd(events_fd, robust);
	if (ret < 0)
		goto fail;

	ret = ftruncate(shm_fd, sizeof(fipc_channel));
	if (ret < 0)
		goto fail;

	// here is the tricky to avoid share memory leak
	ret = shm_unlink(name);
	if (ret < 0)
		goto fail;

	shm_fd2 = dup(shm_fd);
	if (shm_fd2 < 0)
		goto fail;

	events_fd[0].mgmt.shm = shm_fd;
	events_fd[1].mgmt.shm = shm_fd2;
	return 0;


fail:
	if (shm_fd >= 0)
		close(shm_fd);

	if (shm_fd2 >= 0)
		close(shm_fd2);

	for (i = 0; i < 2; i++){
		if (events_fd[i].raw >= 0)
			fipc_close_events_fd(events_fd[i]);
	}

	return -1;
}

int fipc2(int64_t fipcfd[2], int flags, int robust)
{
	int ret = -1;
	int i = 0;

	ret = fipc(fipcfd, robust);
	if (ret < 0)
		return ret;

	for (i = 0; i < 2; i++){
		ret = fipc_fcntl(fipcfd[i], F_SETFD, flags);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int fipc_close(int64_t fd)
{
}

int64_t fipc_dup(int64_t oldfd)
{
}
