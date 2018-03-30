#include <fipc.h>
#include <internal.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>

ssize_t fipc_write(int64_t _fd, void *buf, size_t size)
{
        fipc_fd fd = {.raw = _fd};
        fipc_channel *channel = NULL;
        int64_t idx;
        ssize_t ret_size = 0;
        ssize_t size_left = size;

	if (_fd < 0) {
		errno = EINVAL;
		return -1;
	}

        if (_fd < 0 || !(fd.mgmt.control & FIPC_FD_MASK))
                return write((int)_fd, buf, size);

	if (size > FIPC_CHANNEL_SIZE / 2)
		size = FIPC_CHANNEL_SIZE / 2;

        lock_fd(fd.mgmt.shm);

        channel = get_channel(fd.mgmt.shm);
        if (!channel)
        {
                ret_size = -1;
                goto done;
        }

        while (size_left)
        {
                idx = channel->wt_idx++;
                idx %= FIPC_BLOCK_NUMBER;

                int ret = get_op(fd.mgmt.control & FIPC_FD_MASK)->wait_wte(fd, &channel->blocks[idx]);
		if (ret < 0) {
			channel->wt_idx--;
			ret_size = ret;
			goto done;
		} else if (ret == 0){
			channel->wt_idx--;
			goto done;
		}

		int64_t copy_size = size_left > FIPC_BLOCK_SIZE ? FIPC_BLOCK_SIZE : size_left;
                memcpy(&channel->blocks[idx].buf, (char *)buf + ret_size, copy_size);
                channel->blocks[idx].amount = copy_size;
                channel->blocks[idx].offset = 0;
                atomic_set(&channel->blocks[idx].status, 1);
                ret_size += copy_size;
                size_left -= copy_size;
#ifndef NDEBUG
                atomic_add_and_fetch(&channel->write_size, copy_size);
#endif
        }

done:
        unlock_fd(fd.mgmt.shm);
        return ret_size;
}
