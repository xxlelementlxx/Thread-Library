#include <cstdlib>
#include <ucontext.h>
#include <deque>
#include <iterator>
#include <map>
#include <iostream>
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

void delete_current_thread() {
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
  if (init) return -1;

  init = true;

  if (thread_create(func, arg) < 0)
    return -1;

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
      delete_current_thread();
    Thread* next = ready.front();
    ready.pop_front();
    current = next;
    swapcontext(scheduler, current->ucontext_ptr);
  }

  if (current != NULL) {
    delete_current_thread();
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
    
    t->ucontext_ptr = new ucontext_t;
    getcontext(t->ucontext_ptr);
    
    t->stack = new char [STACK_SIZE];
    t->ucontext_ptr->uc_stack.ss_sp = t->stack;
    t->ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
    t->ucontext_ptr->uc_stack.ss_flags = 0;
    t->ucontext_ptr->uc_link = NULL;
    makecontext(t->ucontext_ptr, (void (*)())exec_func, 2, func, arg);
    
    t->id = id;
    id++;
    t->finished = false;
    ready.push_back(t);
  }
  catch (std::bad_alloc b) {
    delete t->ucontext_ptr;
    delete t->stack;
    delete t;
    interrupt_enable();
    return -1;
  }
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
  if (lock_iter == lock_map.end()) { //New Lock
    try {
      l = new Lock;
      l->owner = current;
      l->blocked_threads = new deque<Thread*>;
    }    
    catch (std::bad_alloc b) {
      delete l->blocked_threads;
      delete l;
      interrupt_enable();
      return -1;
    }
    lock_map.insert(make_pair(lock, l));
  } 
  else { //Lock Found
    l = (*lock_iter).second;
    if (l->owner == NULL) //Lock is free, get it
      l->owner = current;
    else {
      if (l->owner->id == current->id) {
        //Calling lock when holding it = error
        interrupt_enable();
        return -1;
      } 
      else { //Lock is owned by another Thread, get it line
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
  if (lock_iter == lock_map.end()) // Lock Not Found, error
    return -1;
  else { //Lock Found
    l = (*lock_iter).second;
    if (l->owner == NULL) //Lock Free, error
      return -1;
    else { //Lock owned
      if (l->owner->id == current->id) { //Current thread owns the lock
        if (l->blocked_threads->size() > 0) {
          l->owner = l->blocked_threads->front();
          l->blocked_threads->pop_front();
          ready.push_back(l->owner);
        } 
        else
          l->owner = NULL;
      }
      else //Not the owner, error
        return -1;
    }
  }
  return 0;
}

int thread_unlock(unsigned int lock) {
  if (!init) return -1;

  interrupt_disable();
  int result = unlock_without_interrupts(lock);
  interrupt_enable();
  return result;
}

int thread_wait(unsigned int lock, unsigned int cond) {
  if (!init) return -1;

  interrupt_disable();
  if (unlock_without_interrupts(lock) == 0) {
    condition_iterator cond_iter = condition_map.find(cond);
    if (cond_iter == condition_map.end()) { //New Condition variable
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
    } else { //Found Condition variable
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
  if (!init) return -1;

  interrupt_disable();
  condition_iterator cond_iter = condition_map.find(cond);
  if (cond_iter != condition_map.end()) { //Condition Found
    deque<Thread*>* c = (*cond_iter).second;
    if (!c->empty()) { //Threads waiting
      Thread* t = c->front();
      c->pop_front();
      ready.push_back(t);
    }
  }
  //Condition not found/not holding the lock => not an error
  interrupt_enable();
  return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
  if (!init) return -1;

  interrupt_disable();
  condition_iterator cond_iter = condition_map.find(cond);
  if (cond_iter != condition_map.end()) { //Condition Found
    deque<Thread*>* c = (*cond_iter).second;
    while (c->size() > 0) {  //Threads waiting
      Thread* t = c->front();
      c->pop_front();
      ready.push_back(t);
    }
  }
  //Condition not found/not holding the lock => not an error
  interrupt_enable();
  return 0;
}