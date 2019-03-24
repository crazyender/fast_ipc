#include <fcntl.h>
#include <fipc.h>
#include <internal.h>

int fipc_clear_fd_flag(int fd, int flag) {
  int ret = fcntl(fd, F_GETFD);
  if (ret < 0) return ret;

  ret &= ~flag;
  ret = fcntl(fd, F_SETFD, ret);
  if (ret < 0) return ret;

  return 0;
}

#ifdef DEBUG

void fipc_get_dbg_info(int64_t _fd, fipc_debug_info *info) {
  fipc_fd fd = {.raw = _fd};
  fipc_channel *channel = NULL;

  if (!info || _fd < 0 || !(fd.mgmt.control & FIPC_FD_MASK)) return;

  lock_fd_read(fd.mgmt.shm);
  channel = get_channel(fd.mgmt.shm);
  if (!channel) goto done;

  memcpy(info, &channel->dbg, sizeof(*info));
done:

  unlock_fd(fd.mgmt.shm);
}

void fipc_clear_dbg_info(int64_t _fd) {
  fipc_fd fd = {.raw = _fd};
  fipc_channel *channel = NULL;

  if (_fd < 0 || !(fd.mgmt.control & FIPC_FD_MASK)) return;

  lock_fd_read(fd.mgmt.shm);
  channel = get_channel(fd.mgmt.shm);
  if (!channel) goto done;

  memset(&channel->dbg, 0, sizeof(channel->dbg));
done:

  unlock_fd(fd.mgmt.shm);
}

#endif
