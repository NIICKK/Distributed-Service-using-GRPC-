#ifndef _Thread_H_
#define _Thread_H_

#include <queue>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <unistd.h>
#include "Thread.h"

class Thread{
private:
    std::thread _thread;
    bool _isfree;
    std::function<void()> _task;
    std::mutex _locker;
public:
    //Constructor
    Thread();
    //if free
    bool isfree();
    //add task
    void addTask(std::function<void()> task);
    //If there is a task then execute it, otherwise spin
    void run();
};

#endif
