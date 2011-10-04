/*
 * interrupt.h -- interface to manipulate simulated hardware interrupts.
 *
 * This file should be included in the thread library, but NOT in the
 * application program that uses the thread library.
 */
#ifndef _INTERRUPT_H
#define _INTERRUPT_H

/*
 * interrupt_disable() and interrupt_enable() simulate the hardware's interrupt
 * mask.  These functions provide a way to make sections of the thread library
 * code atomic.
 *
 * assert_interrupts_disabled() and assert_interrupts_enabled() can be used
 * as error checks inside the thread library.  They will assert (i.e. abort
 * the program and dump core) if the condition they test for is not met.
 *
 * These functions/macros should only be called in the thread library code.
 * They should NOT be used by the application program that uses the thread
 * library; application code should use locks to make sections of the code
 * atomic.
 */
extern void interrupt_disable(void);
extern void interrupt_enable(void);
extern "C" {extern int test_set_interrupt(void);}

#define assert_interrupts_disabled()					\
		assert_interrupts_private(__FILE__, __LINE__, true)
#define assert_interrupts_enabled()					\
		assert_interrupts_private(__FILE__, __LINE__, false)


/*
 * assert_interrupts_private is a private function for the interrupt library.
 * Your thread library should not call it directly.
 */
extern void assert_interrupts_private(char *, int, bool);

#endif /* _INTERRUPT_H */
