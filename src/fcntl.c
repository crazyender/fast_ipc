#include <fipc.h>
#include <fipc_fcntl.h>
#include <internal.h>

int fipc_fcntl(int64_t _fd, int cmd, int val) {
  fipc_fd fd = {.raw = _fd};
  int ret = -1;

  if (!(fd.mgmt.control & FIPC_FD_MASK)) {
    ret = fcntl((int)_fd, cmd, val);
    goto done;
  }

  if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_PIPE) {
    if (fd.mgmt.rde >= 0) {
      ret = fcntl(fd.mgmt.rde, cmd, val);
      goto done;
    }

    if (fd.mgmt.wte >= 0) {
      ret = fcntl(fd.mgmt.wte, cmd, val);
      goto done;
    }

    return -1;
  } else if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_EVENT) {
    if (fd.mgmt.rde >= 0) {
      ret = fcntl(fd.mgmt.rde, cmd, val);
      if (ret < 0) goto done;
    }

    if (fd.mgmt.wte >= 0) {
      ret = fcntl(fd.mgmt.wte, cmd, val);
    }
    goto done;
  } else if ((fd.mgmt.control & FIPC_FD_MASK) == FIPC_FD_EVENT) {
    goto done;
  }

done:
  if (ret == 0 && cmd == F_SETFL) {
    lock_fd_write(fd.mgmt.shm);
    get_channel(fd.mgmt.shm)->flags = val;
    unlock_fd(fd.mgmt.shm);
  }
  return ret;
}