#include <stdio.h>
#include <fipc.h>
#include <fipc_fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <openssl/md5.h>
#include "fipc_test.h"

#define SEND_BUFFER_SIZE 65536
#define RECV_BUFFER_SIZE 65536

static void read_and_send_file(const char * name, int64_t fd)
{
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_CTX context;
	FILE* file = fopen(name, "r");
	if (!file) die("can not open test file %s", name);

	fseek(file, 0, SEEK_END);
	int64_t filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	int64_t size_left = filesize;

	log_info("begin to transfer data, file size %lld", filesize);

        srand(time(NULL));
        MD5_Init(&context);

	unsigned char *buf = malloc(SEND_BUFFER_SIZE);
	fipc_write(fd, &filesize, sizeof(filesize));// we assume 8 bytes are not possible to fail..

	while (size_left > 0) {
		int64_t read_size = (size_left > SEND_BUFFER_SIZE) ? SEND_BUFFER_SIZE : size_left;
                read_size = rand() % read_size;
		if (read_size == 0)
			read_size = (size_left > SEND_BUFFER_SIZE) ? SEND_BUFFER_SIZE : size_left;
		fread(buf, read_size, 1, file);
                
                MD5_Update(&context, buf, read_size);

		unsigned char *tmp = buf;
		int64_t buf_left = read_size;
		while (buf_left > 0) {
			int64_t ret = fipc_write(fd, tmp, buf_left);
			if (ret < 0)
				die("send fail, size_left %lld, fd %.16llx", size_left, fd);
			buf_left -= ret;
			size_left -= ret;
			tmp += ret;
		}
	}
        printf("\n");
	fclose(file);
	free(buf);

        MD5_Final(hash, &context);
        printf("md5 for %s:\t", name);
	for(int i = 0; i < MD5_DIGEST_LENGTH; ++i)
                printf("%02x", (unsigned int)hash[i]);
        printf("\n");
}

static void receive_and_save_file(const char *name, int64_t fd)
{
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_CTX context;

        MD5_Init(&context);
        srand(time(NULL));

	int64_t filesize = 0;
	fipc_read(fd, &filesize, sizeof(filesize));// we assume 8 bytes are not possible to fail..

	log_info("begin to read, file size %lld", filesize);
	unsigned char *buf = malloc(RECV_BUFFER_SIZE);
	int64_t size_left = filesize;
	while (size_left > 0) {
                int read_size = RECV_BUFFER_SIZE > size_left ? size_left : RECV_BUFFER_SIZE;
		read_size = rand() % read_size;
		if (read_size == 0)
			read_size = RECV_BUFFER_SIZE > size_left ? size_left : RECV_BUFFER_SIZE;
		int64_t ret = fipc_read(fd, buf, read_size);
		if (ret <= 0)
			die("receive fail");
                MD5_Update(&context, buf, ret);
		size_left -= ret;
	}
	free(buf);
        MD5_Final(hash, &context);
	printf("md5 for %s:\t", name);
	for(int i = 0; i < MD5_DIGEST_LENGTH; ++i)
                printf("%02x", (unsigned int)hash[i]);
        printf("\n");

}

int main(int argc, char **argv)
{
        fipc_init();
	if (argc < 2)
		die("usage: %s [read|write] [type] [fd]\n", argv[0]);

	int64_t fd;
	int read_end = 0;
	if (strcmp(argv[1], "read") == 0)
		read_end = 1;
	else if (strcmp(argv[1], "write") == 0)
		read_end = 0;
	else
		die("usage: %s [read|write] [fd]\n", argv[0]);

        int type = atoi(argv[2]);

	int ret = 0;
	int64_t fds[2];

	// open channel
	if (read_end) {
		fd = atoll(argv[3]);
	} else {
		ret = fipc(fds, type);
		if (ret < 0) {
			die("can not acquire channel\n");
		}

		pid_t pid = fork();
		if (pid == 0){
			char str_fd[32] = {0};
                        char str_type[32] = {0};
			sprintf(str_fd, "%ld", fds[0]);
                        sprintf(str_type, "%d", type);
			execl(argv[0], argv[0], "read", str_type, str_fd, (char *)NULL);
		} else {
			fd = fds[1];
		}

	}
	// do data transfer
	if (!read_end) {
		printf("write: fd %.16llx\n", fd);
		read_and_send_file("test.dat", fd);
		int status;
		wait(&status);
		fipc_close(fds[0]);
		fipc_close(fds[1]);
	} else {
                printf("read: fd %.16llx\n", fd);
		receive_and_save_file("test.bak", fd);
		fipc_close(fd);
	}


	// close channel


	return 0;
}