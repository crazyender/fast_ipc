#ifndef _FIPC_POLL_H_
#define _FIPC_POLL_H_

#include <poll.h>
#include <stdint.h>

struct fipc_pollfd {
  int64_t fd;
  short events;
  short revents;
};

int fipc_poll(struct fipc_pollfd *fds, nfds_t nfds, int timeout);

#endif
