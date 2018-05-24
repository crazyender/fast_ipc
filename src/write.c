#include <assert.h>
#include <errno.h>
#include <fipc.h>
#include <internal.h>
#include <memory.h>

ssize_t fipc_write(int64_t _fd, void *buf, size_t size)
{
	fipc_fd fd = {.raw = _fd};
	fipc_channel *channel = NULL;
	int64_t idx;
	ssize_t ret_size = 0;
	ssize_t size_left = 0;

	if (_fd < 0 || !(fd.mgmt.control & FIPC_FD_MASK))
		return write((int)_fd, buf, size);

	size_left = size;

	lock_fd_read(fd.mgmt.shm);

	channel = get_channel(fd.mgmt.shm);
	if (unlikely(!channel))
	{
		ret_size = -1;
		goto done;
	}

	while (likely(size_left))
	{
		idx = channel->wt_idx++;
		idx %= FIPC_BLOCK_NUMBER;

		int ret = get_op(fd.mgmt.control & FIPC_FD_MASK)
					  ->wait_wte(fd, channel);
		if (unlikely(ret < 0))
		{
			channel->wt_idx--;
			if (ret_size == 0)
				ret_size = ret;
			goto done;
		}
		else if (unlikely(ret == 0))
		{
			channel->wt_idx--;
			goto done;
		}

		int64_t copy_size = size_left > FIPC_BLOCK_SIZE
								? FIPC_BLOCK_SIZE
								: size_left;
		memcpy(&channel->blocks[idx].buf, (char *)buf + ret_size,
			   copy_size);
		channel->blocks[idx].amount = copy_size;
		channel->blocks[idx].offset = 0;
		ret_size += copy_size;
		size_left -= copy_size;
		ret = get_op(fd.mgmt.control & FIPC_FD_MASK)
				  ->notify_rde(fd, channel);
		if (ret <= 0)
		{
			// fatal error
			ret_size = -1;
			goto done;
		}
	}

done:
	unlock_fd(fd.mgmt.shm);
	return ret_size;
}
