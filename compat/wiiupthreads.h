#ifndef COMPAT_WIIUPTHREADS_H
#define COMPAT_WIIUPTHREADS_H


#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>


#include "libavutil/attributes.h"
#include "libavutil/common.h"
#include "libavutil/internal.h"
#include "libavutil/mem.h"
#include "libavutil/time.h"


#include <coreinit/atomic.h>
#include <coreinit/atomic64.h>
#include <coreinit/thread.h>
#include <coreinit/mutex.h>
#include <coreinit/condition.h>


#include <wut.h>

#define PTHREAD_ONCE_INIT { 1, 0 }

#define PTHREAD_MUTEX_INITIALIZER _PTHREAD_MUTEX_INITIALIZER

#define OS_THREAD_DEF_PRIO     0x10




static av_unused int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
   if (!OSTestAndSetAtomic((uint32_t *)&once_control->is_initialized, 1)) {
      init_routine();
      once_control->init_executed = 1;
   }
   while (!once_control->init_executed)
      OSYieldThread();
   return 0;
}

static void thread_deallocator(OSThread *thread, void *stack) {
   free(stack);
   free(thread);
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
   OSInitMutex((OSMutex *)(*mutex = (pthread_mutex_t)calloc(1, sizeof(OSMutex))));
   return 0;
}

static void check_mutex(pthread_mutex_t *mutex) {
   if (*mutex == _PTHREAD_MUTEX_INITIALIZER)
      pthread_mutex_init(mutex, NULL);
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex) {
   if (*mutex != _PTHREAD_MUTEX_INITIALIZER)
      free((OSMutex *)*mutex);
   *mutex = 0;
   return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
   check_mutex(mutex);
   OSLockMutex((OSMutex *)*mutex);
   return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
   check_mutex(mutex);
   OSUnlockMutex((OSMutex *)*mutex);
   return 0;
}

static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
   OSInitCond((OSCondition *)(*cond = (pthread_cond_t)malloc(sizeof(OSCondition))));
   return 0;
}
static inline int pthread_cond_destroy(pthread_cond_t *cond) {
   free((void *)*cond);
   *cond = 0;
   return 0;
}

static inline  int pthread_cond_signal(pthread_cond_t *cond) {
   // OSSignalCond is actually a broadcast,
   // this is fine tough because pthread allows that.
   OSSignalCond((OSCondition *)*cond);
   return 0;
}
static inline  int pthread_cond_broadcast(pthread_cond_t *cond) {
   OSSignalCond((OSCondition *)*cond);
   return 0;
}
static inline  int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
   OSWaitCond((OSCondition *)*cond, (OSMutex *)*mutex);
   return 0;
}

static inline int pthread_create(pthread_t *pthread, const pthread_attr_t *pthread_attr, void *(*start_routine)(void *), void *arg) {
   OSThread *thread = calloc(1, sizeof(OSThread));

   int stack_size = 4 * 1024 * 1024;
   if (pthread_attr && pthread_attr->stacksize)
      stack_size = pthread_attr->stacksize;

   void *stack_addr = NULL;
   if (pthread_attr && pthread_attr->stackaddr)
      stack_addr = (uint8_t *)pthread_attr->stackaddr + pthread_attr->stacksize;
   else
      stack_addr = (uint8_t *)memalign(8, stack_size) + stack_size;

   OSThreadAttributes attr = OS_THREAD_ATTRIB_AFFINITY_ANY;
      attr = OS_THREAD_ATTRIB_AFFINITY_CPU0 | OS_THREAD_ATTRIB_AFFINITY_CPU2;
   if (pthread_attr && pthread_attr->detachstate)
      attr |= OS_THREAD_ATTRIB_DETACHED;

   if (!OSCreateThread(thread, (OSThreadEntryPointFn)start_routine, (int)arg, NULL, stack_addr, stack_size, OS_THREAD_DEF_PRIO, attr))
      return EINVAL;

   OSSetThreadDeallocator(thread, thread_deallocator);
   *pthread = (pthread_t)thread;
   OSResumeThread(thread);

   return 0;
}

static inline int pthread_join(pthread_t pthread, void **value_ptr) {
   return !OSJoinThread((OSThread *)pthread, (int *)value_ptr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif