#include <fipc.h>
#include <internal.h>
#include <sched.h>
#include <errno.h>

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

int spin_wait_rde(fipc_fd fd, fipc_block *block)
{
	int retries = 0;
	int relax = 1;

	while(1) {
		if (block->status != 0){
			if (atomic_get(&block->status) != 0)
				break;
		}

		if (fd.mgmt.rde == -2){
			errno = EAGAIN;
			return -1;
		}
		usleep(relax++);
/*
		retries++;
                if (retries < 200)
                        continue;
                else if (retries < 2000)
                        sched_yield();
                else
                        usleep(20 * 1000);
*/
	}

        return 1;
}

int spin_wait_wte(fipc_fd fd, fipc_block *block)
{
	int retries = 0;
	int relax = 1;

	while(1) {
		if (block->status == 0){
			if (atomic_get(&block->status) == 0)
				break;
		}

		if (fd.mgmt.wte == -2){
			errno = EAGAIN;
			return -1;
		}
		usleep(relax++);
/*
		retries++;
                if (retries < 200)
                        continue;
                else if (retries < 2000)
                        sched_yield();
                else
                        usleep(20 * 1000);
*/
	}

        return 1;
}

int spin_poll(struct fipc_pollfd *fds, nfds_t nfds, int timeout)
{
        // FIXME
        return -1;
}

fipc_op spin_op = {
        .open = spin_open,
        .close = spin_close,
        .wait_rde = spin_wait_rde,
        .wait_wte = spin_wait_wte,
        .poll = spin_poll
};
