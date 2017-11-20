#include "Thread.h"

Thread::Thread()
	: _isfree(true), _task(nullptr){
        _thread = std::thread(&Thread::run, this);
        _thread.detach(); //put the thread to the backgroud， join: waitting for the end of thread execution
}

bool Thread::isfree(){
     return _isfree;
}

void Thread::addTask(std::function<void()> task){
     if(_isfree){
         std::lock_guard<std::mutex> guard (_locker);
         _task = task;
         _isfree = false;
     }
}

void Thread::run(){
     while(true){
         if(_task){
             std::lock_guard<std::mutex> guard (_locker);
             _isfree = false;
             _task();
             _isfree = true;
             _task = nullptr;
         }
     }
}
