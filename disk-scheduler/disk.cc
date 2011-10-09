#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <deque>
#include <fstream>
#include <algorithm>
#include "thread.h"
#include "interrupt.h"

using namespace std;

struct Request {
  int id;
  int track;
};

int mutex, diskqFull, diskqOpen, diskqLimit, numThreads, lastTrack;
char** filenames;
bool* serviced;
deque<Request*> diskq;

int abs(int i) { return (i > 0) ? i : -i; }
bool sortfunc(Request* i, Request* j) {
  return (abs(i->track - lastTrack) < abs(j->track - lastTrack));
}
bool isDiskqFull() { return diskq.size() >= diskqLimit; }

void sendRequest(Request* r) {
  cout << "requester " << r->id << " track " << r->track << endl;
  diskq.push_back(r);
  sort(diskq.begin(), diskq.end(), sortfunc);
  serviced[r->id] = false;
}

void serviceRequest() {
  Request* r = diskq.front();
  diskq.pop_front();
  cout << "service requester " << r->id << " track " << r->track << endl;
  serviced[r->id] = true;
  lastTrack = r->track;
  delete r;
}

void requester(void* id) {
  int requesterID = (int)id;
  ifstream in(filenames[requesterID]);
  int track = 0;
  Request* r;
  
  while (true) {
    in >> track;
    if(in.eof()) break;
    
    r = new Request;
    r->id = requesterID;
    r->track = track;
    thread_lock(mutex);
    
    while (isDiskqFull() || serviced[r->id] == false)
      thread_wait(mutex, diskqOpen);

    sendRequest(r);
    thread_broadcast(mutex, diskqFull);
  }
  in.close();
  
  while (serviced[requesterID] == false)
    thread_wait(mutex, diskqOpen);
  
  numThreads--;
  if (numThreads < diskqLimit) {
    diskqLimit--;
    thread_broadcast(mutex, diskqFull);
  }
  thread_unlock(mutex); 
}

void service(void* arg) {
  thread_lock(mutex);
  while (diskqLimit > 0) {
    while (!isDiskqFull()) 
      thread_wait(mutex, diskqFull);
  
    if (!diskq.empty())
      serviceRequest();
  
    thread_broadcast(mutex, diskqOpen);
  }
  thread_unlock(mutex);
}

void master(void* arg) {
  thread_create(service, NULL);

  for (int i = 0; i < numThreads; i++)
    thread_create(requester, (void*)i);
}

int main(int argc, char** argv) {
  if (argc <= 2) 
    return 0;
  
  diskqLimit = atoi(argv[1]);
  numThreads = argc - 2;
  lastTrack = 0;
  mutex = 1;
  diskqFull = 2;
  diskqOpen = 3;

  serviced = new bool[numThreads];
  for (int i = 0; i < numThreads; i++)
    serviced[i] = true;

  filenames = argv + 2;
  thread_libinit(&master, NULL);
  
  delete serviced;
  return 0;
}