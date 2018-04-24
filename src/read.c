#include <assert.h>
#include <errno.h>
#include <fipc.h>
#include <internal.h>
#include <memory.h>

ssize_t fipc_read(int64_t _fd, void *buf, size_t size)
{
	fipc_fd fd = {.raw = _fd};
	fipc_channel *channel = NULL;
	int64_t idx;
	ssize_t ret_size = 0;
	ssize_t size_left = size;
	int is_again = 0;
	int is_error = 0;

	if (_fd < 0)
	{
		errno = EINVAL;
		return -1;
	}

	if (_fd < 0 || !(fd.mgmt.control & FIPC_FD_MASK))
		return read((int)_fd, buf, size);

	lock_fd_read(fd.mgmt.shm);

	channel = get_channel(fd.mgmt.shm);
	if (!channel)
	{
		ret_size = -1;
		goto done;
	}

	while (likely(size_left))
	{
		int ret;
		ssize_t copy_size;
		if (unlikely(channel->backlog.amount))
		{
			copy_size = channel->backlog.amount > size_left
							? size_left
							: channel->backlog.amount;
			memcpy((char *)buf + ret_size,
				   channel->backlog.buf + channel->backlog.offset,
				   copy_size);
			channel->backlog.amount -= copy_size;
			channel->backlog.offset += copy_size;
			size_left -= copy_size;
			ret_size += copy_size;
			continue;
		}
		idx = channel->rd_idx++;
		idx %= FIPC_BLOCK_NUMBER;
		ret = get_op(fd.mgmt.control & FIPC_FD_MASK)
				  ->wait_rde(fd, channel);
		if (unlikely(ret < 0))
		{
			channel->rd_idx--;
			if (ret_size == 0)
				ret_size = ret;
			goto done;
		}
		else if (unlikely(ret == 0))
		{
			channel->rd_idx--;
			goto done;
		}

		copy_size = size_left > channel->blocks[idx].amount
						? channel->blocks[idx].amount
						: size_left;
		memcpy((char *)buf + ret_size, channel->blocks[idx].buf,
			   copy_size);

		channel->blocks[idx].amount -= copy_size;
		if (channel->blocks[idx].amount)
		{
			memcpy(channel->backlog.buf,
				   channel->blocks[idx].buf + copy_size,
				   channel->blocks[idx].amount);
			channel->backlog.amount = channel->blocks[idx].amount;
			channel->backlog.offset = 0;
			channel->blocks[idx].amount = 0;
			channel->blocks[idx].offset = 0;
		}
		size_left -= copy_size;
		ret_size += copy_size;
#ifndef NDEBUG
		atomic_add_and_fetch(&channel->read_size, copy_size);
#endif
		ret = get_op(fd.mgmt.control & FIPC_FD_MASK)
				  ->notify_wte(fd, channel);
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
