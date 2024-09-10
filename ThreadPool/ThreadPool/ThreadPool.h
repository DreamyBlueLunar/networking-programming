//
// Created by LesPaul on 9/9/2024.
//

#ifndef THREAD_POOL_THREADPOOL_H
#define THREAD_POOL_THREADPOOL_H

#include "../TaskQueue/TaskQueue.h"
#include <pthread.h>

class ThreadPool {
public:
    ThreadPool(int min, int max);
    ~ThreadPool();

    void addTask(Task task);

private:
    static void *worker(void *arg);
    static void *manager(void *arg);
    void threadExit();

private:
    TaskQueue* m_taskQ;
    pthread_t* m_threadIDs;
    pthread_t m_managerID;
    pthread_mutex_t m_lock;
    pthread_cond_t m_notEmpty;
    int m_aliveNum;
    int m_minNum;
    int m_maxNum;
    int m_exitNum;
    int m_busyNum;
    bool m_shutdown;
};


#endif //THREAD_POOL_THREADPOOL_H
