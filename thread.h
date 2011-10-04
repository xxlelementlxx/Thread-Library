/*
 * thread.h -- public interface to thread library
 *
 * This file should be included in both the thread library and application
 * programs that use the thread library.
 */
#ifndef _THREAD_H
#define _THREAD_H

#define STACK_SIZE 262144	/* size of each thread's stack */

typedef void (*thread_startfunc_t) (void *);

extern int thread_libinit(thread_startfunc_t func, void *arg);
extern int thread_create(thread_startfunc_t func, void *arg);
extern int thread_yield(void);
extern int thread_lock(unsigned int lock);
extern int thread_unlock(unsigned int lock);
extern int thread_wait(unsigned int lock, unsigned int cond);
extern int thread_signal(unsigned int lock, unsigned int cond);
extern int thread_broadcast(unsigned int lock, unsigned int cond);

/*
 * start_preemptions() can be used in testing to configure the generation
 * of interrupts (which in turn lead to preemptions).
 *
 * The sync and async parameters allow several styles of preemptions:
 *
 *     1. async = true: generate asynchronous preemptions every 10 ms using
 *        SIGALRM.  These are non-deterministic.
 *
 *     2. sync = true: generate synchronous, pseudo-random preemptions before
 *        interrupt_disable and after interrupt_enable.  You can generate
 *        different (but deterministic) preemption patterns by changing
 *        random_seed.
 *
 * start_preemptions() should be called (at most once) in the application
 * function started by thread_libinit().  Make sure this is after the thread
 * system is done being initialized.
 *
 * If start_preemptions() is not called, no interrupts will be generated.
 *
 * The code for start_preemptions is in interrupt.cc, but the declaration
 * is in thread.h because it's part of the public thread interface.
 */
extern void start_preemptions(bool async, bool sync, int random_seed);

#endif /* _THREAD_H */
