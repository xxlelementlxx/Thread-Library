/*
 * File:   thread.cpp
 * Author: Yang Su
 *
 * Created on February 1, 2011, 10:35 PM
 */

#include <cstdlib>
#include <ucontext.h>
#include <deque>
#include <iostream>
#include <iterator>
#include <map>
#include <assert.h>
#include "interrupt.h"
#include "thread.h"

using namespace std;

struct Thread {
    unsigned int id;
    char* stack;
    ucontext_t* ucontext_ptr;
    bool finished;
};

struct Lock {
    Thread* owner;
    deque<Thread*>* blocked_threads;
};

typedef map<unsigned int, deque<Thread*>*>::const_iterator condition_iterator;
typedef map<unsigned int, Lock*>::const_iterator lock_iterator;
typedef void (*thread_startfunc_t)(void *);

static Thread* current;
static ucontext_t* scheduler;
static deque<Thread*> ready;
static map<unsigned int, deque<Thread*>*> condition_map;
static map<unsigned int, Lock*> lock_map;

static bool init = false;
static int id = 0;

void delete_current() {
    delete current->stack;
    current->ucontext_ptr->uc_stack.ss_sp = NULL;
    current->ucontext_ptr->uc_stack.ss_size = 0;
    current->ucontext_ptr->uc_stack.ss_flags = 0;
    current->ucontext_ptr->uc_link = NULL;
    delete current->ucontext_ptr;
    delete current;
    current = NULL;
}

int thread_libinit(thread_startfunc_t func, void *arg) {
    if (init) {
        return -1;
    }
    interrupt_disable();
    init = true;
    interrupt_enable();

    if (thread_create(func, arg) < 0) {
        return -1;
    }

    //Set current thread
    Thread* first = ready.front();
    ready.pop_front();
    current = first;

    try {
        scheduler = new ucontext_t;
    } catch (std::bad_alloc b) {
        delete scheduler;

        return -1;
    }
    getcontext(scheduler);
    
    //switch to current thread
    interrupt_disable();
    swapcontext(scheduler, first->ucontext_ptr);

    while (ready.size() > 0) {
        if (current->finished == true)
            delete_current();
        Thread* next = ready.front();
        ready.pop_front();
        current = next;
        swapcontext(scheduler, next->ucontext_ptr);
    }

    if (current != NULL) {
        delete_current();
    }

    //When there are no runnable threads in the system (e.g. all threads have
    //finished, or all threads are deadlocked)
    cout << "Thread library exiting.\n";
    exit(0);
}

static int exec_func(thread_startfunc_t func, void *arg) {
    interrupt_enable();
    func(arg);
    interrupt_disable();

    current->finished = true;
    swapcontext(current->ucontext_ptr, scheduler);
    return 0;
}

int thread_create(thread_startfunc_t func, void *arg) {
    if (!init) return -1;
    
    interrupt_disable();
    Thread* t;
    try {
        t = new Thread;
    } 
    catch (std::bad_alloc b) {
        delete t;
        interrupt_enable();
        return -1;
    }

    t->id = id;
    id++;
    t->finished = false;

    try {
        t->ucontext_ptr = new ucontext_t;
        getcontext(t->ucontext_ptr);
    } 
    catch (std::bad_alloc b) {
        delete t->ucontext_ptr;
        delete t;
        interrupt_enable();
        return -1;
    }

    try {
        t->stack = new char [STACK_SIZE];
    } 
    catch (std::bad_alloc b) {
        delete t->ucontext_ptr;
        delete t->stack;
        delete t;
        interrupt_enable();
        return -1;
    }
    t->ucontext_ptr->uc_stack.ss_sp = t->stack;
    t->ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
    t->ucontext_ptr->uc_stack.ss_flags = 0;
    t->ucontext_ptr->uc_link = NULL;

    makecontext(t->ucontext_ptr, (void (*)())exec_func, 2, func, arg);

    ready.push_back(t);
    interrupt_enable();
    return 0;
}

int thread_yield(void) {
    if (!init) return -1;
    
    interrupt_disable();
    ready.push_back(current);
    swapcontext(current->ucontext_ptr, scheduler);
    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    if (!init) return -1;

    interrupt_disable();
    lock_iterator lock_iter = lock_map.find(lock);
    Lock* l;
    if (lock_iter == lock_map.end()) {

        try {
            l = new Lock;
        } catch (std::bad_alloc b) {
            delete l;
            interrupt_enable();
            return -1;
        }

        l->owner = current;
        try {
            l->blocked_threads = new deque<Thread*>;
        }        catch (std::bad_alloc b) {
            delete l->blocked_threads;
            delete l;
            interrupt_enable();
            return -1;
        }
        lock_map.insert(make_pair(lock, l));
    } 
    else {
        l = (*lock_iter).second;
        if (l->owner == NULL) {
            l->owner = current;
        } 
        else {
            if (l->owner->id == current->id) {
                interrupt_enable();
                return -1;
            } else {
                l->blocked_threads->push_back(current);
                swapcontext(current->ucontext_ptr, scheduler);
            }
        }
    }
    interrupt_enable();

    return 0;
}

int unlock_without_interrupts(unsigned int lock) {
    lock_iterator lock_iter = lock_map.find(lock);
    Lock* l;
    if (lock_iter == lock_map.end()) {
        interrupt_enable();
        return -1;
    } 
    else {
        l = (*lock_iter).second;
        if (l->owner == NULL) {
            interrupt_enable();
            return -1;
        } 
        else {
            if (l->owner->id == current->id) {
                //move 1 blocked to owner
                if (l->blocked_threads->size() > 0) {

                    l->owner = l->blocked_threads->front();
                    l->blocked_threads->pop_front();
                    ready.push_back(l->owner);
                } else {
                    l->owner = NULL;
                }
            } 
            else {
                interrupt_enable();
                return -1;
            }
        }
    }

    return 0;
}

int thread_unlock(unsigned int lock) {
    if (!init) return -1;

    interrupt_disable();
    lock_iterator lock_iter = lock_map.find(lock);
    Lock* l;
    if (lock_iter == lock_map.end()) {
        interrupt_enable();
        return -1;
    } 
    else {
        l = (*lock_iter).second;
        if (l->owner == NULL) {
            interrupt_enable();
            return -1;
        } 
        else {
            if (l->owner->id == current->id) {
                if (l->blocked_threads->size() > 0) {
                    l->owner = l->blocked_threads->front();
                    l->blocked_threads->pop_front();
                    ready.push_back(l->owner);
                } 
                else {
                    l->owner = NULL;
                }
            }
            else {
                interrupt_enable();
                return -1;
            }
        }
    }
    interrupt_enable();
    return 0;
}

static int test_lock(unsigned int lock) {
    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    if (!init) return -1;

    interrupt_disable();
    if (unlock_without_interrupts(lock) == 0) {
        condition_iterator cond_iter = condition_map.find(cond);
        if (cond_iter == condition_map.end()) {
            //not found in the condition map => new condition
            deque<Thread*>* waiting_threads;
            try {
                waiting_threads = new deque<Thread*>;
            } 
            catch (std::bad_alloc b) {
                delete waiting_threads;
                interrupt_enable();
                return -1;
            }
            waiting_threads->push_back(current);
            condition_map.insert(make_pair(cond, waiting_threads));
        } else {
            (*cond_iter).second->push_back(current);
        }
        swapcontext(current->ucontext_ptr, scheduler);
        interrupt_enable();
        return thread_lock(lock);
    } 
    else 
        return -1;
}

int thread_signal(unsigned int lock, unsigned int cond) {
    if (!init)
        return -1;

    interrupt_disable();

    if (test_lock(lock) == 0) {
        //lock exists and current thread owns the lock
        condition_iterator cond_iter = condition_map.find(cond);
        if (cond_iter == condition_map.end()) {
            //not found in the condition map
            interrupt_enable();
            return 0;
        } else {
            deque<Thread*>* c = (*cond_iter).second;
            if (c->empty()) {
                //no threads waiting
                interrupt_enable();
                return 0;
            } else {
                Thread* t = c->front();
                c->pop_front();
                ready.push_back(t);
            }
        }
    } else {
        //error in lock
        interrupt_enable();
        return 0;
    }

    interrupt_enable();
    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    if (!init)
        return -1;

    interrupt_disable();
    if (test_lock(lock) == 0) {
        //lock exists and current thread owns the lock
        condition_iterator cond_iter = condition_map.find(cond);
        if (cond_iter == condition_map.end()) {
            //not found in the condition map
            interrupt_enable();
            return 0;
        } else {
            deque<Thread*>* c = (*cond_iter).second;
            if (c->empty()) {
                //no threads waiting
                interrupt_enable();
                return 0;
            } else {
                while (c->size() > 0) {
                    Thread* t = c->front();
                    c->pop_front();
                    ready.push_back(t);
                }
            }
        }
    } else {
        //error in lock
        interrupt_enable();
        return 0;
    }

    interrupt_enable();
    return 0;
}