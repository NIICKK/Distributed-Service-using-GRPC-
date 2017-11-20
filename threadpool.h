#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "Thread.h"

class ThreadPool{
private:
    std::queue<std::function<void()>> taskQueue;
    std::vector<Thread *> myPool;
    std::mutex myLocker;

public:
    //Constructor,execute in the background
    ThreadPool(int n);
    //desctruct the threadpool
    ~ThreadPool();
    //add task to the queue
    void addTask(std::function<void()> task);
    //polling
    void run();
    //size of the queue
    int queueSize();
};

#endif
