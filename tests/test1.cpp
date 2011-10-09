#include <cstdlib>
#include <iostream>
#include "thread.h"
using namespace std;

int g = 0;
int count = 0;
unsigned int lock = 1, cond = 1;

void
loop(void *a) {
    char *id;
    int i;

    id = (char *) a;
    cout << "loop called with id " << (char *) id << endl;

    for (i = 0; i < 5; i++, g++) {
        cout << id << ":\t" << i << "\t" << g << endl;
        if (thread_yield()) {
            cout << "thread_yield failed\n";
            exit(1);
        }
    }
}

void
loop2(void *a) {
   thread_lock(lock);
    count++;
    cout << " blocked count " << count << endl;
    thread_wait(lock, cond);

    cout << " blocked count " << count << endl;
    count--;
   thread_unlock(lock);
   thread_signal(lock,cond);

}

void wake_signal(void*a)
{
    thread_signal(lock,cond);
}

void wake_broadcast(void*a)
{
    thread_broadcast(lock,cond);
}

void
parent(void *a) {
    int arg;
    arg = (int) a;
    start_preemptions(false, true, 1);
    cout << "parent called with arg " << arg << endl;
    for (int i = 0; i < 5; i++) {
        if (thread_create((thread_startfunc_t) loop, (void *) "child thread")) {
            cout << "thread_create failed\n";
            exit(1);
        }
    }

    loop((void *) "parent thread");

    cout<<"creating threads and signal"<<endl;
    for (int i = 0; i < 5; i++) {
        if (thread_create((thread_startfunc_t) loop2, (void *) "signal thread")) {
            cout << "thread_create failed\n";
            exit(1);
        }
    }

    cout<<"creating threads and broadcast" <<endl;
    thread_create((thread_startfunc_t) wake_signal, (void *) "wake thread");

//    for (int i = 0; i < 5; i++) {
//        if (thread_create((thread_startfunc_t) loop2, (void *) "broadcast thread")) {
//            cout << "thread_create failed\n";
//            exit(1);
//        }
//    }
//    thread_create((thread_startfunc_t) wake_broadcast, (void *) "wake thread");
}

int
main() {
    if (thread_libinit((thread_startfunc_t) parent, (void *) 100)) {
        cout << "thread_libinit failed\n";
        exit(1);
    }
}