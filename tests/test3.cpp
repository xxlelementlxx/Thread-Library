#include <cstdlib>
#include <iostream>
#include <deque>
#include <algorithm>
#include "thread.h"

using namespace std;

struct Request {
    unsigned int rqstr;
    int track;
};

unsigned int cout_lock = 230, id = 0;
unsigned int full_cond = 21341, open_cond = 2323;


unsigned int max_disk_queue = 3;
unsigned int active_threads = 5;
int current_track = 0;

bool* flags;
deque<Request*> diskq;

unsigned int vals[5][2] = {
    {53, 785},
    {914, 350},
    {827, 567},
    {302, 230},
    {631, 11}
};

int abs(int i) {
    return (i > 0) ? i : -i;
}

bool myfunction(Request* i, Request* j) {
    return (abs(i->track - current_track) < abs(j->track - current_track));
}

bool is_full() {
    return diskq.size() >= max_disk_queue;
}

Request* get_object() {
    Request* r = diskq.front();
    diskq.pop_front();
    return r;
}

void request(Request* r) {
    cout << "requester " << r->rqstr << " track " << r->track << endl;
    diskq.push_back(r);
    sort(diskq.begin(), diskq.end(), myfunction);
    flags[r->rqstr] = false;
}

void service_request() {
    Request* r = get_object();
    cout << "service requester " << r->rqstr << " track " << r->track << endl;
    flags[r->rqstr] = true;
    current_track = r->track;
}

void requester(void* f) {
    unsigned int index = (unsigned int) f;
    unsigned int i = 0;
    Request* r;

    while (i < 2) {
        r = new Request;
        r->rqstr = index;
        r->track = vals[r->rqstr][i];


        cout<<thread_lock(cout_lock);
        cout << "LOCK: thread " << index << endl;
        while (is_full() || flags[r->rqstr] == false) {
            cout << "WAIT: thread " << index << endl;
            thread_wait(cout_lock, open_cond);
        }
        request(r);
        i++;
        cout << "BROADCAST: thread " << index << endl;
        cout<<thread_broadcast(cout_lock, full_cond);
    }

    //    thread_lock(cout_lock);
    while (flags[r->rqstr] == false) {
        cout << "WAIT: thread " << index << endl;
        thread_wait(cout_lock, open_cond);
    }
    //    cout<<"active_threads "<<active_threads<<endl;
    active_threads--;

    if (active_threads < max_disk_queue) {
        max_disk_queue--;
        cout << "MD BROADCAST: thread " << index << endl;
        cout<<thread_broadcast(cout_lock, full_cond);

        //        cout<<"max_disk_queue "<<max_disk_queue<<endl;
    }
    cout << "UNLOCK: thread " << index << endl;
    thread_unlock(cout_lock);
    cout << "EXIT: thread " << index << endl;
}

void service(void* arg) {
    cout<<thread_lock(cout_lock); //obtain lock
    cout << "LOCK: service thread" << endl;
    while (max_disk_queue > 0) {
        while (!is_full()) {
            cout << "WAIT: service thread" << endl;
            thread_wait(cout_lock, full_cond);
        }
        //service
        if (!diskq.empty())
            service_request();

        //                cout<<"max_disk_queue"<<max_disk_queue<<endl;
        //unlock
        cout << "BROADCAST: service thread" << endl;
        cout<<thread_broadcast(cout_lock, open_cond);
    }
    cout << "UNLOCK: service thread" << endl;
    cout<<thread_unlock(cout_lock);
    cout << "EXIT: service thread" << endl;
}

void master(void* arg) {
    //Create service thread
    thread_create(service, arg);
    for (unsigned int i = 0; i < 5; i++) {
        void* num = (void*) id;
        id++;
        thread_create(requester, num);
    }
}

int main(int argc, char** argv) {
    flags = new bool[5];
    for (unsigned int i = 0; i < 5; i++) {
        flags[i] = true;
    }

    thread_libinit(&master, (void *) "");
}