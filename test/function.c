#include <stdio.h>
#include <fipc.h>
#include <fipc_fcntl.h>
#include <stdlib.h>
#include <memory.h>

static void test_open_close(int type)
{
        int ret;
        
        int64_t fds[2];

        ret = fipc(fds, type);
        printf("fipc(%.16lx, %.16lx, %d) = %d\n", fds[0], fds[1], type, ret);

        ret = fipc_close(fds[0]);
        printf("fipc_close(%.16lx) = %d\n", fds[0], ret);

        ret = fipc_close(fds[1]);
        printf("fipc_close(%.16lx) = %d\n", fds[1], ret);
}

static void test_fcntl(int type)
{
        int ret;
        
        int64_t fds[2];

        ret = fipc(fds, type);

        ret = fipc_fcntl(fds[0], F_SETFL, O_NONBLOCK);
        printf("fipc_fcntl(%.16lx, F_SETFL, O_NONBLOCK) = %d\n", fds[0], ret);

        ret = fipc_close(fds[0]);

        ret = fipc_close(fds[1]);
}

static void test_read_write(int type, int size)
{
        ssize_t ret;
        int64_t fds[2];
        char *msg = NULL;
        char *dst = NULL;

        msg = malloc(size);
        dst = malloc(size);

        memset(msg, 0, size);
        memset(dst, 0, size);
        strcpy(msg, "hello, world");

        fipc2(fds, O_NONBLOCK, type);
        fipc_fcntl(fds[0], F_SETFL, O_NONBLOCK);
        fipc_fcntl(fds[1], F_SETFL, O_NONBLOCK);
        ret = fipc_write(fds[1], msg, size);
        printf("fipc_write(%.16lx, %s, %d) = %ld\n", fds[1], msg, size, ret);

        ret = fipc_read(fds[0], dst, size);
        printf("fipc_read(%.16lx, %s, %d) = %ld\n", fds[0], dst, size, ret);


        fipc_close(fds[0]);
        fipc_close(fds[1]);

        free(msg);
        free(dst);
}

int main()
{
        int i = 0;
        fipc_init();


        test_open_close(FIPC_FD_PIPE);
        test_open_close(FIPC_FD_EVENT);
        test_open_close(FIPC_FD_SPIN);

        test_fcntl(FIPC_FD_PIPE);
        test_fcntl(FIPC_FD_EVENT);
        test_fcntl(FIPC_FD_SPIN);

        for (i = 4096; i <= 4 * 1024 * 1024; i *= 2){
                test_read_write(FIPC_FD_PIPE, i);
                test_read_write(FIPC_FD_EVENT, i);
                test_read_write(FIPC_FD_SPIN, i);
        }
        return 0;
}
