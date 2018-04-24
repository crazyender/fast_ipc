#include <fipc.h>
#include <internal.h>
#ifdef __linux__
#include <fcntl.h>
#include <sys/eventfd.h>

int eventfd_open(fipc_fd fds[2])
{
        int rdfd = -1;
        int wtfd = -1;
        int rdfd2 = -1;
        int wtfd2 = -1;
        int ret;

        /* using eventfd */
        rdfd = eventfd(0, EFD_SEMAPHORE);
        if (rdfd < 0)
                goto fail;

        wtfd = eventfd(FIPC_BLOCK_NUMBER, EFD_SEMAPHORE);
        if (wtfd < 0)
                goto fail;

        ret = fipc_clear_fd_flag(rdfd, FD_CLOEXEC);
        if (ret < 0)
                goto fail;

        fipc_clear_fd_flag(wtfd, FD_CLOEXEC);
        if (ret < 0)
                goto fail;

        fds[0].mgmt.control = FIPC_FD_EVENT;
        fds[0].mgmt.rde = rdfd;
        fds[0].mgmt.wte = wtfd;
        fds[1].mgmt.control = FIPC_FD_EVENT;
        rdfd2 = dup(rdfd);
        wtfd2 = dup(wtfd);
        if (rdfd2 < 0 || wtfd2 < 0)
                goto fail;

        fds[1].mgmt.rde = rdfd2;
        fds[1].mgmt.wte = wtfd2;

        return 0;

fail:
        if (rdfd >= 0)
                close(rdfd);

        if (wtfd >= 0)
                close(wtfd);

        if (rdfd2 >= 0)
                close(rdfd2);

        if (wtfd2 >= 0)
                close(wtfd2);

        return -1;
}

int eventfd_close(fipc_fd fd)
{
        int ret = 0;
        if (fd.mgmt.rde >= 0)
                ret = (close(fd.mgmt.rde) < 0);

        if (fd.mgmt.wte >= 0)
                ret |= (close(fd.mgmt.wte) < 0);

        if (ret)
                return -1;
        else
                return 0;
}

int eventfd_wait_rde(fipc_fd fd, fipc_channel *channel)
{
        int64_t event = 1;

        // decreate read quota
        event = 1;
        return read(fd.mgmt.rde, &event, sizeof(event));
}

int eventfd_notify_wte(fipc_fd fd, fipc_channel *channel)
{
        int64_t event = 1;
        // increate write quota
        event = 1;
        return write(fd.mgmt.wte, &event, sizeof(event));
}

int eventfd_wait_wte(fipc_fd fd, fipc_channel *channel)
{
        int64_t event = 1;

        // decreate write quota
        event = 1;
        return read(fd.mgmt.wte, &event, sizeof(event));
}

int eventfd_notify_rde(fipc_fd fd, fipc_channel *channel)
{
        int64_t event = 1;
        // increate read quota
        event = 1;
        return write(fd.mgmt.rde, &event, sizeof(event));
}

int eventfd_poll(struct fipc_pollfd *fds, nfds_t nfds, int timeout)
{
        // FIXME
        return -1;
}

fipc_op eventfd_op = {
    .open = eventfd_open,
    .close = eventfd_close,
    .wait_rde = eventfd_wait_rde,
    .notify_wte = eventfd_notify_wte,
    .wait_wte = eventfd_wait_wte,
    .notify_rde = eventfd_notify_rde,
    .poll = eventfd_poll};
#endif