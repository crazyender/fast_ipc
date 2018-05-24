//
//  Share memory IPC
//
//  Created by Ender Zheng in 2018-1
//  Copyright (c) 2018 Agora IO. All rights reserved.
//
#define _GNU_SOURCE
#include "fipc_test.h"
#include <fcntl.h>
#include <fipc.h>
#include <fipc_debug.h>
#include <fipc_poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static int64_t threads = 1;
static int64_t send_chunk_size = 4096;
static int64_t recv_chunk_size = 4096;
static int64_t loops = 10;
static int64_t dir = 1;
static int64_t help_needed = 0;
static int64_t nohead = 0;
static int64_t read_fd = -1;
static int64_t type = 1;

static int64_t test_times = 0;

static void help(const char *exec_name)
{
	log_info("usage: %s [args]", exec_name);
	log_info("\t threads=[threads] (default 1)");
	log_info("\t size=[size] (default 4096)");
	log_info("\t recv_chunk_size=[size] (default 4096)");
	log_info("\t loops=[loop] (default 10)");
	log_info("\t nohead=[0|1] (default 0)");
	log_info("\t type=[1|2|3|4] (1:pipe;2:event_fd;3:spin at "
		 "userland;4:legacy pipe)(default 1)");
}

static int64_t get_val_from_opt(char *arg, const char *opt)
{
	char *tmp = strstr(arg, opt);
	tmp += strlen(opt);
	return atoll(tmp);
}

static void parse_arg(char *arg, const char *opt, int64_t *val)
{
	if (strstr(arg, opt) == arg)
		*val = get_val_from_opt(arg, opt);
}

static int64_t write_all_buffer(int64_t id, void *buf, int64_t size)
{
	int64_t size_left = size;
	char *tmp = (char *)buf;
	while (size_left > 0) {
		int64_t ret = fipc_write(id, tmp, size_left);
		if (ret == 0)
			break;

		if (ret < 0)
			return -1;

		size_left -= ret;
		tmp += ret;
	}
	return (int64_t)(tmp - (char *)buf);
}

struct perf_thread_p
{
	int64_t channel;
	int64_t read_size;
	int64_t write_size;
	pid_t remote_pid;
};

static int wait_for_channel(int64_t id, int mode)
{
	// struct pollfd fds;

	// fds.fd = id;
	// if (mode == 0) {
	// 	fds.events |= POLLIN | POLLRDHUP | POLLHUP | POLLERR;
	// } else {
	// 	fds.events |= POLLOUT | POLLRDHUP | POLLHUP | POLLERR;
	// }

	// int ret = fipc_poll(&fds, 1, -1);
	// if (ret <= 0)
	// 	return -1;

	// if (fds.revents & (POLLHUP | POLLRDHUP | POLLERR | POLLNVAL))
	// 	return -1;

	// return 0;
}

static int perf_thread_callback(void *param)
{
	struct perf_thread_p *p = (struct perf_thread_p *)param;
	int64_t id = p->channel;

	if (dir == 0) { // read
		int64_t ret;

		char *buf = malloc(recv_chunk_size);
		while (1) {
			ret = fipc_read(id, buf, recv_chunk_size);
			if (ret == 0) {
				break;
			}
			if (ret < 0)
				break;

			p->read_size += ret;
		}

		free(buf);

		fipc_close(id);

	} else { // write

		char *buf = malloc(send_chunk_size);
		memset(buf, 0xfc, send_chunk_size);
		int64_t ret;
		while (1) {
			ret = write_all_buffer(id, buf, send_chunk_size);
			if (ret == 0) {
				break;
			}
			if (ret < 0)
				break;

			p->write_size += ret;
		}
		free(buf);

		fipc_close(id);
	}
	free(p);
	return 0;
}

static int timer_callback_fun(int real_dur, void *p)
{
	struct perf_thread_p **param = (struct perf_thread_p **)p;
	int64_t total_read = 0;
	int64_t total_write = 0;
	int64_t total_mem = 0;
#ifdef DEBUG
	fipc_debug_info total = {0};
	fipc_debug_info info = {0};
#endif
	for (int i = 0; i < threads; i++) {
		total_read += param[i]->read_size;
		total_write += param[i]->write_size;
		param[i]->read_size = 0;
		param[i]->write_size = 0;
#ifdef DEBUG
		fipc_get_dbg_info(param[i]->channel, &info);
		fipc_clear_dbg_info(param[i]->channel);
		total.read_contents += info.read_contents;
		total.write_contents += info.write_contents;
		total.syscalls += info.syscalls;
		total.backlog_used += info.backlog_used;
#endif
	}

	int64_t read_perf
		= real_dur ? (total_read * 1000 / (real_dur * 1024 * 1024)) : 0;
	int64_t write_perf = real_dur
		? (total_write * 1000 / (real_dur * 1024 * 1024))
		: 0;

	printf("%ld,%d,%ld,%ld", total_mem / 1024, real_dur, read_perf,
		write_perf);
#ifdef DEBUG
	printf(",%ld,%ld,%ld,%ld", total.read_contents, total.write_contents, total.syscalls, total.backlog_used);
#endif
	printf("\n");
	test_times++;
	if ((test_times >= loops) || (total_read == 0 && total_write == 0)) {
		for (int i = 0; i < threads; i++) {
			kill(param[i]->remote_pid, SIGKILL);
		}
		exit(0);
	}
	return 0;
}
static int configure_pipe_size(int fd)
{
#ifdef F_SETPIPE_SZ
	int ret = fcntl(fd, F_SETPIPE_SZ, 1 * 1024 * 1024);
	if (ret < 0) {
		return ret;
	}
	return 0;
#else

#endif
	return 0;
}

static int fipc_clear_fd_flag(int fd, int flag)
{
	int ret = fcntl(fd, F_GETFD);
	if (ret < 0)
		return ret;

	ret &= ~flag;
	ret = fcntl(fd, F_SETFD, ret);
	if (ret < 0)
		return ret;

	return 0;
}

int main(int argc, char **argv)
{
	fipc_init();
	if (argc == 1) {
		help(argv[0]);
		exit(0);
	}

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "help") == 0) {
			help(argv[0]);
			exit(0);
		}

		parse_arg(argv[i], "dir=", &dir);
		parse_arg(argv[i], "threads=", &threads);
		parse_arg(argv[i], "size=", &send_chunk_size);
		parse_arg(argv[i], "recv_chunk_size=", &recv_chunk_size);
		parse_arg(argv[i], "loops=", &loops);
		parse_arg(argv[i], "nohead=", &nohead);
		parse_arg(argv[i], "read_fd=", &read_fd);
		parse_arg(argv[i], "type=", &type);
	}

	signal(SIGPIPE, SIG_IGN);

	// open share memory
	if (dir) {
		if (!nohead) {
			printf("MemoryUsage(kb),"
			       "Duration(ms),"
			       "ReadPerf(mb/s),"
			       "WritePerf(mb/s)");
#ifdef DEBUG
			printf(",ReadContents,"
			       "WriteContents,"
			       "SystemCalls,"
			       "BacklogUsed");
#endif
			printf("\n");
		}
	}
	if (dir == 1) {
		void **all_threads = malloc(sizeof(void *) * threads);
		pid_t *all_pids = malloc(sizeof(pid_t) * threads);
		void **all_param = malloc(sizeof(void *) * threads);
		for (int i = 0; i < threads; i++) {
			int64_t fds[2];
			if (type != 4) {
				fipc2(fds, 0, type);
			} else {
				int pipefd[2];
				pipe(pipefd);
				fipc_clear_fd_flag(pipefd[0], O_NONBLOCK);

				configure_pipe_size(pipefd[0]);
				fipc_clear_fd_flag(pipefd[0], FD_CLOEXEC);
				fds[0] = pipefd[0];
				fds[1] = pipefd[1];
			}
			pid_t pid = fork();
			if (pid == 0) {
				char str_dir[32];
				char str_chunk[32];
				char str_fd[32];

				sprintf(str_dir, "dir=%ld", 1 - dir);
				sprintf(str_chunk, "recv_chunk_size=%ld",
					send_chunk_size);
				sprintf(str_fd, "read_fd=%ld", fds[0]);
				fipc_close(fds[1]);
				execl(argv[0], argv[0], str_dir, str_chunk, str_fd, (char *)NULL);
			} else {
				fipc_close(fds[0]);
				all_pids[i] = pid;
				struct perf_thread_p *p
					= malloc(sizeof(struct perf_thread_p));
				p->channel = fds[1];
				p->read_size = p->write_size = 0;
				p->remote_pid = pid;
				all_threads[i] = create_thread(
					perf_thread_callback, (void *)p);
				all_param[i] = (void *)p;
			}
		}

		void *timer = create_timer(1000, timer_callback_fun, all_param);

		for (int i = 0; i < threads; i++) {
			join_thread(all_threads[i]);
			int status;
			waitpid(all_pids[i], &status, 0);
		}
		stop_timer(timer);
		free(all_threads);
		free(all_pids);
		free(all_param);
	} else {
		struct perf_thread_p *p = malloc(sizeof(struct perf_thread_p));
		p->channel = read_fd;
		perf_thread_callback((void *)p);
	}

	return 0;
}
