#include <cstdlib>
#include <iostream>
#include "thread.h"
using namespace std;

int num = 1;
int count = 0;
int stop = 10;
unsigned int cout_lock = 123;
unsigned int lock2 = 1;
unsigned int lock_dne = 1;
unsigned int change = 123;
unsigned int cond_dne = 123;
void print_num() {
    cout << "current num: " << num << " current count: " << count << endl;
}

void check_errors() {
    if (thread_lock(cout_lock) == 0)
        cout << "ERROR: acquired lock when already holding the lock\n";
    if (thread_unlock(lock2) == 0)
        cout << "ERROR: unlocked lock owned by four\n";
    if (thread_unlock(lock_dne) == 0)
        cout << "ERROR: unlocked lock owned by four\n";
    if (thread_wait(lock_dne, cond_dne) == 0)
        cout << "ERROR: waiting on a lock that doesn't exist\n";
    if (thread_wait(lock2, cond_dne) == 0)
        cout << "ERROR: waiting on a lock that's not owned by the thread\n";
    if (thread_wait(lock_dne, change) == 0)
        cout << "ERROR: waiting on a lock that doesn't exist\n";
    if (thread_wait(lock2, change) == 0)
        cout << "ERROR: waiting on a lock that's not owned by the thread\n";
    if (thread_signal(cout_lock, cond_dne)!=0)
        cout << "ERROR: incorrect handling on non-existent condition\n";
    if (thread_broadcast(cout_lock, cond_dne)!=0)
        cout << "ERROR: incorrect handling on non-existent condition\n";
    if (thread_signal(lock_dne, change)!=0)
        cout << "ERROR: incorrect handling on non-existent condition\n";
    if (thread_broadcast(lock_dne, change)!=0)
        cout << "ERROR: incorrect handling on non-existent condition\n";
    if (thread_signal(lock2, change)!=0)
        cout << "ERROR: incorrect handling on non-existent condition\n";
    if (thread_broadcast(lock2, change)!=0)
        cout << "ERROR: incorrect handling on non-existent condition\n";

}

void one(void *a) {
    cout << "one started\n";
    while (count < stop) {
        cout<<thread_lock(cout_lock);
        check_errors();
        while (num != 1)
            thread_wait(cout_lock, change);
        cout << "one acquires lock\n";
        count++;
        print_num();
        num = 2;
        cout<<thread_broadcast(cout_lock, change);
        cout<<thread_unlock(cout_lock);
        thread_yield();
    }
    cout << "one finished\n";
}

void two(void *a) {
    cout << "two started\n";
    while (count < stop) {
        cout<<thread_lock(cout_lock);
        while (num != 2){
            check_errors();
            thread_wait(cout_lock, change);
        }
        cout << "two acquires lock\n";

        count++;
        print_num();
        num = 3;
        cout<<thread_broadcast(cout_lock, change);
        cout<<thread_unlock(cout_lock);
        thread_yield();
    }
    cout << "two finished\n";
}

void three(void *a) {
    cout << "three started\n";
    while (count < stop) {
        cout<<thread_lock(cout_lock);
        while (num != 3)
            thread_wait(cout_lock, change);

        cout << "three acquires lock\n";
        check_errors();
        count++;
        print_num();
        num = 1;
        cout<<thread_broadcast(cout_lock, change);
        cout<<thread_unlock(cout_lock);
        thread_yield();
    }
    cout << "three finished\n";
}

void four(void *a) {
    cout << "four started\n";
    cout<<thread_lock(lock2);
    cout << "four acquires lock2\n";
    cout<<thread_lock(cout_lock);

    while (count < stop - 1) {
        thread_wait(cout_lock, change);
    }
    cout << "four acquires lock\n";
    cout << count << " is odd" << endl;

    cout<<thread_broadcast(cout_lock, change);
    cout<<thread_unlock(cout_lock);
    cout<<thread_unlock(lock2);
    thread_yield();

    cout << "four finished\n";
}

void test_init(int (*func)(unsigned int)) {
    if (func(cout_lock))
        cout << "thread lib not initialized\n";
    else
        cout << "ERROR: function success with uninitialized library\n";
}

void test_init2(int (*func)(unsigned int, unsigned int)) {
    if (func(cout_lock, change))
        cout << "thread lib not initialized\n";
    else
        cout << "ERROR: function success with uninitialized library\n";
}

void test_init3(int (*func)(void)) {
    if (func())
        cout << "thread lib not initialized\n";
    else
        cout << "ERROR: function success with uninitialized library\n";
}

void test_method_initialization() {
    test_init(&thread_lock);
    test_init(&thread_unlock);
    test_init2(&thread_wait);
    test_init2(&thread_signal);
    test_init2(&thread_broadcast);
    test_init3(&thread_yield);

}

void reinit_thread(void* a) {
    cout << "ERROR: Initialized threadlib twice!!\n";
}

void master(void* a) {
    stop = (int) a;
//    start_preemptions(false,true,1);
    thread_create((thread_startfunc_t) one, (void*) "nothing");
    thread_create((thread_startfunc_t) two, (void*) "nothing");
    thread_create((thread_startfunc_t) three, (void*) "nothing");
    thread_create((thread_startfunc_t) four, (void*) "nothing");

    thread_libinit((thread_startfunc_t) reinit_thread, (void *) 1);
}

int main() {
    test_method_initialization();
    thread_libinit((thread_startfunc_t) master, (void *) 10);
}