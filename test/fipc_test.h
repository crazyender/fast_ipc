#ifndef SHM_TEST_H
#define SHM_TEST_H
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MB (1024 * 1024)

#ifndef WIN32
#define SHM_NAME "/test_shm"
#define TEST_FILE "./test_file"
#else
#define SHM_NAME "Local\\test_shm"
#define TEST_FILE "c:\\test_file"
#endif

typedef int (*async_callback)(void *param);
struct thread_p {
  async_callback callback;
  void *param;
  void *handle;
};

#ifndef WIN32
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <unistd.h>

#define YIELD() sched_yield()
static void msleep(long ms) {
  struct timespec sp;
  sp.tv_sec = ms / 1000;
  sp.tv_nsec = (ms - sp.tv_sec * 1000) * 1000000;
  nanosleep(&sp, NULL);
}

static long get_current_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void *thread_routine(void *param) {
  struct thread_p *p = (struct thread_p *)param;
  p->callback(p->param);
  return 0;
}

static void *create_thread(async_callback callback, void *param) {
  pthread_t id;
  struct thread_p *p = malloc(sizeof(struct thread_p));
  p->callback = callback;
  p->param = param;
  pthread_create(&id, NULL, thread_routine, p);
  p->handle = (void *)id;
  return p;
}

static void join_thread(void *thread) {
  struct thread_p *p = (struct thread_p *)thread;
  pthread_join((pthread_t)p->handle, NULL);
  free(p);
}

#else
#include <Windows.h>

#define YIELD() SwitchToThread()

static long get_current_time() { return (long)GetTickCount64(); }

static void msleep(long ms) { Sleep((DWORD)ms); }

static DWORD WINAPI WinThreadProc(_In_ LPVOID lpParameter) {
  thread_p *p = (thread_p *)lpParameter;
  p->callback(p->param);
  return 0;
}

static void *create_thread(async_callback callback, void *param) {
  thread_p *p = new thread_p();
  p->callback = callback;
  p->param = param;
  p->handle = CreateThread(NULL, 0, WinThreadProc, p, CREATE_SUSPENDED, NULL);
  ResumeThread(p->handle);
  return p;
}

static void join_thread(void *thread) {
  thread_p *p = (thread_p *)thread;
  WaitForSingleObject(p->handle, INFINITE);
  CloseHandle(p->handle);
  delete p;
}

#endif

static void log_msg(const char *prefix, const char *msg) {
  printf("%s", prefix);
  printf(": ");
  printf("%s\n", msg);
}

static void log_error(const char *fmt, ...) {
  printf("ERROR: ");

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  printf("\n");
}

static void log_pass(const char *fmt, ...) {
  printf("PASS: ");

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  printf("\n");
}

static void log_info(const char *fmt, ...) {
  printf("INFO: ");

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);

  printf("\n");
}

static void die(const char *fmt, ...) {
  va_list ap;
  printf("FATAL: ");
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
  exit(-1);
}

typedef int (*timer_callback)(int real_dur, void *p);

struct my_timer_t {
  int stopped;
  void *thread;
  long interval;
  long last_invoke_time;
  timer_callback callback;
  void *param;
};

static int my_timer_thread_callback(void *p) {
  struct my_timer_t *t = (struct my_timer_t *)p;
  while (1) {
    msleep(t->interval);
    if (t->stopped) break;
    t->callback(get_current_time() - t->last_invoke_time, t->param);
    t->last_invoke_time = get_current_time();
  }
  return 0;
}

static void *create_timer(long interval, timer_callback callback, void *param) {
  struct my_timer_t *t = malloc(sizeof(*t));
  t->stopped = 0;
  t->interval = interval;
  t->last_invoke_time = get_current_time();
  t->callback = callback;
  t->param = param;
  t->thread = create_thread(my_timer_thread_callback, t);
  return t;
}

static void stop_timer(void *timer) {
  struct my_timer_t *t = (struct my_timer_t *)timer;
  t->stopped = 1;
  join_thread(t->thread);
  free(t);
}

#endif