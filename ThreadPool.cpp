#include "threadpool.h"

ThreadPool::ThreadPool(int n){
     while(n--){
         Thread *t = new Thread();
         //Create a (size=n) thread pool
         myPool.push_back(t);
     }
     std::thread main_thread(&ThreadPool::run, this);
     main_thread.detach();
}

ThreadPool::~ThreadPool(){
     for(int i = 0;i < myPool.size(); ++i){
         delete myPool[i];
     }
}

void ThreadPool::addTask(std::function<void()> task){
     std::lock_guard<std::mutex> guard (myLocker);
     taskQueue.push(task);
}

void ThreadPool::run(){
     while(true){
         std::lock_guard<std::mutex> guard (myLocker);
         if(taskQueue.empty()){
             continue;
         }
         // looking for free thread to execute task
         for(int i = 0; i < myPool.size(); ++i){
             if(myPool[i]->isfree()){
                 myPool[i]->addTask(taskQueue.front());
                 taskQueue.pop();
                 break;
             }
         }
     }
}

int ThreadPool::queueSize(){
     return taskQueue.size();
}
