#ifndef _FIPC_SELECT_H_
#define _FIPC_SELECT_H_

#include <sys/select.h>



int fipc_select(int nfds, fips_fd_set *readfds,
		fipc_fd_set *writefds,
		fipc_fd_set *exceptfds,
		struct timeval *timeout)

#endif
