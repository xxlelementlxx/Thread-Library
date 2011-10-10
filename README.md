## Thread Library ([Detailed Specification](http://www.cs.duke.edu/~chase/cps110/projects/1/project1.txt))
### Programming Interface 
This is a C++ thread library designed to allow for writing multi-threaded programs on a linux machine.

    int thread_libinit(thread_startfunc_t func, void *arg)

**thread_libinit** initializes the thread library. A user program should call thread\_libinit exactly once before calling any other thread functions.
    
    int thread_create(thread_startfunc_t func, void *arg)

**thread_create** is used to create a new thread. When the newly created
thread starts, it will call the function pointed to by func and pass it the
single argument arg.

    int thread_yield(void)

**thread_yield** causes the current thread to yield the CPU to the next runnable thread.  It has no effect if there are no other runnable threads.

    int thread_lock(unsigned int lock)
    int thread_unlock(unsigned int lock)
    int thread_wait(unsigned int lock, unsigned int cond)
    int thread_signal(unsigned int lock, unsigned int cond)
    int thread_broadcast(unsigned int lock, unsigned int cond)

**thread\_lock**, **thread\_unlock**, **thread\_wait**, **thread\_signal**, and **thread\_broadcast** implement [Mesa](http://en.wikipedia.org/wiki/Monitor_\(synchronization\)) monitors in this thread library. 

Each of these functions returns -1 on failure.  Each of these functions
returns 0 on success, except for **thread_libinit**, which does not return
at all on success.

### Error Handling

Here is a list of behaviors that are **NOT** considered errors: 

- signaling without holding the lock (this is explicitly **NOT** an error in Mesa monitors)
- deadlock (however, trying to acquire a lock by a thread that already has the lock **IS** an error)
- a thread that exits while still holding a lock (the thread should
keep the lock)

## Disk Scheduler

The disk scheduler is sample program that simulates how an operating system gets and schedules disk I/Os for multiple threads. Threads issue disk requests by queueing them at the disk scheduler.

The program creates a number of requester threads to issue disk requests and 1 thread to service disk requests. Each requester thread **must** wait until the servicing thread finishes handling its last request before issuing its next request. A requester thread finishes after all the requests in its input file have been serviced.

Requests in the disk queue are NOT serviced in FIFO order but instead in **SSTF** order (shortest seek time first).

The service thread keeps the disk queue as full as possible to minimize average seek distance and only handles a request when the disk queue has the largest possible number of requests.

## Running tests (On a linux machine)

    ./runtests.sh

