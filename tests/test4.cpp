#include <cstdlib>
#include <iostream>
#include "thread.h"
using namespace std;

int num = 1;
int count = 0;
int stop = 10;
int sleepcount=0;
unsigned int cout_lock = 123;
unsigned int lock2 = 1;
unsigned int lock3 =3;
unsigned int lock_dne = 1;
unsigned int change = 123;
unsigned int cond_dne = 123;
void print_num() {
    cout << "current num: " << num << " current count: " << count << endl;
}

void check_errors() {
    if (thread_lock(cout_lock) == 0)
        cout << "acquired cout_lock when already holding the lock\n";
    else
        cout << "failed to acquire cout_lock\n";

    if (thread_unlock(lock2) == 0)
        cout << "unlocked lock2\n";
    else
        cout << "failed unlocked lock2\n";

    if (thread_unlock(lock_dne) == 0)
        cout << "unlocked lock_dne\n";
    else
        cout << "failed to unlock lock_dne\n";

    if (thread_wait(lock_dne, cond_dne) == 0)
        cout <<"waiting on lock_dne and cond_dne\n";
    else
        cout <<"failed to wait on lock_dne and cond_dne\n";

    if (thread_wait(lock2, cond_dne) == 0)
        cout <<"waiting on lock2, cond_dne\n";
    else
        cout <<"failed to wait on lock2, cond_dne\n";

    if (thread_wait(lock_dne, change) == 0)
        cout <<"waiting on lock_dne, change\n";
    else
        cout <<"failed to wait on lock_dne, change\n";

    if (thread_wait(lock_dne, cond_dne) == 0)
        cout <<"waiting on lock_dne and cond_dne\n";
    else
        cout <<"failed to wait on lock_dne and cond_dne\n";

    if (thread_wait(lock2, change) == 0)
        cout <<"waiting on lock2, change\n";
    else
        cout <<"failed to wait on lock2, change\n";

    if (thread_signal(cout_lock, cond_dne)!=0)
        cout << "incorrect handling on non-existent condition\n";
    else
        cout << "signaled cout_lock, cond_dne\n";
    if (thread_broadcast(cout_lock, cond_dne)!=0)
        cout << "incorrect handling on non-existent condition\n";
    else
        cout << "broadcast cout_lock, cond_dne\n";

    if (thread_signal(lock_dne, change)!=0)
        cout << "incorrect handling on non-existent condition\n";
    else
        cout << "signaled lock_dne, change\n";
    if (thread_broadcast(lock_dne, change)!=0)
        cout << "incorrect handling on non-existent condition\n";
    else
        cout << "broadcast lock_dne, change\n";

    if (thread_signal(lock2, change)!=0)
        cout << "incorrect handling on non-existent condition\n";
    else
        cout << "signaled lock2, change\n";
    if (thread_broadcast(lock2, change)!=0)
        cout << "incorrect handling on non-existent condition\n";
    else
        cout << "broadcast lock2, change\n";

}

void one(void *a) {
    cout << "one started\n";
    while (count < stop) {
        thread_lock(cout_lock);
        sleepcount++;
        check_errors();
        while (num != 1){
            thread_wait(cout_lock, change);
        }
        sleepcount--;
        cout << "one acquires lock\n";
        count++;
        print_num();
        num = 2;
        thread_broadcast(cout_lock, change);
        thread_unlock(cout_lock);
        thread_yield();
    }
    cout << "one finished\n";
}

void two(void *a) {
    cout << "two started\n";
    while (count < stop) {
        thread_lock(cout_lock);
        sleepcount++;
        while (num != 2){
            check_errors();
            thread_wait(cout_lock, change);
        }
        cout << "two acquires lock\n";
        sleepcount--;
        count++;
        print_num();
        num = 3;
        thread_broadcast(cout_lock, change);
        thread_unlock(cout_lock);
        thread_yield();
    }
    cout << "two finished\n";
}

void three(void *a) {
    cout << "three started\n";
    while (count < stop) {
        sleepcount++;
        thread_lock(cout_lock);
        while (num != 3)
            thread_wait(cout_lock, change);

        cout << "three acquires lock\n";
        sleepcount--;
        count++;
        print_num();
        num = 1;
        check_errors();
        thread_broadcast(cout_lock, change);
        thread_unlock(cout_lock);
        thread_yield();
    }
    cout << "three finished\n";
}

void four(void *a) {
    cout << "four started\n";
    thread_lock(lock2);
    cout << "four acquires lock2\n";
    thread_lock(cout_lock);

    cout << "acquire lock2 again"<<thread_lock(lock2)<<"\n";
    while (count < stop - 1) {
        thread_wait(cout_lock, change);
    }
    cout << "four acquires lock\n";
    cout << count << " is odd" << endl;

    thread_broadcast(cout_lock, change);
    thread_unlock(cout_lock);
    thread_unlock(lock2);
    cout << "acquire lock2 again"<<thread_unlock(lock2)<<"\n";
    thread_yield();

    cout << "four finished\n";
}

void test_init(int (*func)(unsigned int)) {
    if (func(cout_lock))
        cout << "thread lib not initialized\n";
    else
        cout << "function success with uninitialized library\n";
}

void test_init2(int (*func)(unsigned int, unsigned int)) {
    if (func(cout_lock, change))
        cout << "thread lib not initialized\n";
    else
        cout << "function success with uninitialized library\n";
}

void test_init3(int (*func)(void)) {
    if (func())
        cout << "thread lib not initialized\n";
    else
        cout << "function success with uninitialized library\n";
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
    cout << "Initialized threadlib twice!!\n";
}

void master(void* a) {
    stop = (int) a;
//    start_preemptions(false,true,1);
    thread_create((thread_startfunc_t) four, (void*) "nothing");
    thread_create((thread_startfunc_t) one, (void*) "nothing");
    thread_create((thread_startfunc_t) two, (void*) "nothing");
    thread_create((thread_startfunc_t) three, (void*) "nothing");

    thread_libinit((thread_startfunc_t) reinit_thread, (void *) 1);
}

int main() {
    test_method_initialization();
    thread_libinit((thread_startfunc_t) master, (void *) 10);
}