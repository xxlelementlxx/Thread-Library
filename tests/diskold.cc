#include <cstdlib>
#include <iostream>
#include <deque>
#include <fstream>
#include <algorithm>
#include "thread.h"
#include "interrupt.h"

using namespace std;

struct Request {
    unsigned int rqstr;
    int track;
};

struct args {
    int argc;
    char** argv;
};

struct file {
    int id;
    char* filename;
};

unsigned int  cout_lock = 230, 
              id = 0,
              full_cond = 21341, 
              open_cond = 2323;

unsigned int max_disk_queue;
unsigned int active_threads;
int current_track = 0;

bool* flags;
deque<Request*> diskq;

int abs(int i) { return (i > 0) ? i : -i; }

bool sortfunc(Request* i, Request* j) {
    return (abs(i->track - current_track) < abs(j->track - current_track));
}

bool is_full() { return diskq.size() >= max_disk_queue; }

void print_q() {
    //cout << current_track << endl;
    for (unsigned int i = 0; i < diskq.size(); i++) {
        cout << i << ":" << diskq.at(i)->track << " ";
    }
    cout << endl;
}

Request* get_object() {
    Request* r = diskq.front();
    diskq.pop_front();
    return r;
}

void request(Request* r) {
    cout << "requester " << r->rqstr << " track " << r->track << endl;
    diskq.push_back(r);
    //
    sort(diskq.begin(), diskq.end(), sortfunc);
    flags[r->rqstr] = false;
    //print_q() ;
}

void service_request() {
    Request* r = get_object();
    cout << "service requester " << r->rqstr << " track " << r->track << endl;
    flags[r->rqstr] = true;
    current_track = r->track;
}

void requester(void* ff) {
    file* f = (file*) ff;
    const char * fname = (char*) f->filename;
    unsigned int requester_id = f->id;
    delete f;

    ifstream in(fname);

    char str[255];
    Request* r;
    while (in) {
        in.getline(str, 255);
        if (in) {
            r = new Request;
            r->rqstr = requester_id;
            r->track = atoi(str);

            thread_lock(cout_lock);
            while (is_full() || flags[r->rqstr] == false) {
                thread_wait(cout_lock, open_cond);
            }
            request(r);

            thread_broadcast(cout_lock, full_cond);
        }
    }
    in.close();

    while (flags[r->rqstr] == false) {
        thread_wait(cout_lock, open_cond);
    }
    active_threads--;

    if (active_threads < max_disk_queue) {
        max_disk_queue--;
        thread_broadcast(cout_lock, full_cond);
    }
    thread_unlock(cout_lock);
}

void service(void* arg) {
    thread_lock(cout_lock); //obtain lock
    while (max_disk_queue > 0) {
        while (!is_full()) 
            thread_wait(cout_lock, full_cond);

        if (!diskq.empty())
            service_request();

        thread_broadcast(cout_lock, open_cond);
    }
    thread_unlock(cout_lock);
}

void master(void* arg) {
    args* a = (args*) arg;

    max_disk_queue = atoi(a->argv[1]);
    active_threads = a->argc - 2;

    //Create service thread
    thread_create(service, arg);

    for (unsigned int i = 2; i < a->argc; i++) {
        file* f = new file;
        f->id = id;
        f->filename = a->argv[i];
        id++;
        void* ff = f;
        thread_create(requester, ff);
    }
    delete a;
}

int main(int argc, char** argv) {
    flags = new bool[argc - 2];
    for (unsigned int i = 0; i < (argc - 2); i++)
        flags[i] = true;

    args* a = new args;
    a->argc = argc;
    a->argv = argv;
    void* arg = a;
    thread_libinit(&master, arg);
}