#include <fipc.h>
#include <fipc_fcntl.h>

int fipc_fcntl(int64_t _fd, int cmd, int val)
{
	fipc_fd fd = { .raw = _fd };
	int ret = -1;

	if (!(fd.mgmt.control & FIPC_FD_MASK))
		return fcntl((int)_fd, cmd, val);

	if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_PIPE) {
		if (fd.mgmt.rde >= 0)
			return fcntl(fd.mgmt.rde, cmd, val);

		if (fd.mgmt.wte >= 0)
			return fcntl(fd.mgmt.wte, cmd, val);

		return -1;
	} else if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_EVENT) {
		if (fd.mgmt.rde >= 0) {
			ret = fcntl(fd.mgmt.rde, cmd, val);
			if (ret < 0)
				return ret;
		}

		if (fd.mgmt.wte >= 0) {
			ret = fcntl(fd.mgmt.wte, cmd, val);
			if (ret < 0)
				return ret;
		}

		return 0;
	} else if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_EVENT) {
		return 0;
	}

	return -1;
}