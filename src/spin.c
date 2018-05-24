#include <errno.h>
#include <fipc.h>
#include <internal.h>
#include <sched.h>
#include <fcntl.h>

int spin_open(fipc_fd fds[2])
{
	int ret;

	fds[0].mgmt.control = FIPC_FD_SPIN;
	fds[0].mgmt.rde = -1;
	fds[0].mgmt.wte = -1;
	fds[1].mgmt.control = FIPC_FD_SPIN;
	fds[1].mgmt.rde = -1;
	fds[1].mgmt.wte = -1;
	return 0;
}

int spin_close(fipc_fd fd)
{
	return 0;
}

int spin_wait_rde(fipc_fd fd, fipc_channel *channel)
{
	int retries = 0;
	int relax = 1;
	fipc_block *block = &channel->blocks[(channel->rd_idx - 1) % FIPC_BLOCK_NUMBER];

	while (1)
	{
		if (atomic_get(&block->status) != 0)
		{
			break;
		}

		if (fd.mgmt.rde == -2)
		{
			errno = EAGAIN;
			return -1;
		}
#ifdef DEBUG
		atomic_inc(&channel->dbg.write_contents);
#endif
		retries++;
		if (retries < 5000)
			continue;
		else
		{
			usleep(relax++);
			if (relax > 100 && fcntl(fd.mgmt.shm, F_GETFL) < 0)
				return -1;
		}
	}

	return 1;
}

int spin_notify_wte(fipc_fd fd, fipc_channel *channel)
{
	fipc_block *block = &channel->blocks[(channel->rd_idx - 1) % FIPC_BLOCK_NUMBER];
	atomic_set(&block->status, 0);
	return 1;
}

int spin_wait_wte(fipc_fd fd, fipc_channel *channel)
{
	int retries = 0;
	int relax = 1;
	fipc_block *block = &channel->blocks[(channel->wt_idx - 1) % FIPC_BLOCK_NUMBER];

	while (1)
	{
		if (atomic_get(&block->status) == 0)
		{
			break;
		}

		if (fd.mgmt.wte == -2)
		{
			errno = EAGAIN;
			return -1;
		}
#ifdef DEBUG
		atomic_inc(&channel->dbg.read_contents);
#endif
		retries++;
		if (retries < 5000)
			continue;
		else
		{
			usleep(relax++);
			if (relax > 100 && fcntl(fd.mgmt.shm, F_GETFL) < 0)
				return -1;
		}
	}

	return 1;
}

int spin_notify_rde(fipc_fd fd, fipc_channel *channel)
{
	fipc_block *block = &channel->blocks[(channel->wt_idx - 1) % FIPC_BLOCK_NUMBER];
	atomic_set(&block->status, 1);
	return 1;
}

int spin_poll(struct fipc_pollfd *fds, nfds_t nfds, int timeout)
{
	// FIXME
	return -1;
}

fipc_op spin_op = {.open = spin_open,
				   .close = spin_close,
				   .wait_rde = spin_wait_rde,
				   .wait_wte = spin_wait_wte,
				   .notify_rde = spin_notify_rde,
				   .notify_wte = spin_notify_wte,
				   .poll = spin_poll};
