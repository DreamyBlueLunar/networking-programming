//
// Created by LesPaul on 9/9/2024.
//

// question1: 创建的子线程数量不对：
// A: worker函数设置有问题，导致线程池出现段错误

#include "ThreadPool.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

#define DEBUG_LEVEL 0

ThreadPool::ThreadPool(int minNum, int maxNum) {
    m_taskQ = new TaskQueue();

    do {
        m_minNum = minNum;
        m_maxNum = maxNum;
        m_aliveNum = minNum;
        m_shutdown = false;

        m_threadIDs = new pthread_t[maxNum];
        if (nullptr == m_threadIDs) {
            #if DEBUG_LEVEL
                std::cout << "failed to allocate memory for m_threadIDs(pthread_t *)" << std::endl;
            #endif
            break;
        }
        memset(m_threadIDs, 0, (sizeof (pthread_t)) * maxNum);
        if (0 != pthread_mutex_init(&m_lock, nullptr)) {
            #if DEBUG_LEVEL
                std::cout << "failed to initialize m_lock(pthread_mutex_t)" << std::endl;
            #endif
            break;
        }
        if (0 != pthread_cond_init(&m_notEmpty, nullptr)) {
            #if DEBUG_LEVEL
                std::cout << "failed to initialize m_notEmpty(pthread_cond_t)" << std::endl;
            #endif
            break;
        }

        for (int i = 0; i < minNum; i ++) {
            pthread_create(&m_threadIDs[i], nullptr, worker, this);
            #if DEBUG_LEVEL
                std::cout << "###" << i << ": child thread ID -> " << std::to_string(m_threadIDs[i]) << std::endl;
            #endif
        }

        pthread_create(&m_managerID, nullptr, manager, this);
    } while (false);
}

ThreadPool::~ThreadPool() {
    m_shutdown = true;

    pthread_join(m_managerID, nullptr);
    for (int i = 0; i < m_aliveNum; i ++) {
        pthread_cond_signal(&m_notEmpty);
    }

    if (m_taskQ) {
        delete m_taskQ;
    }
    if (m_threadIDs) {
        delete[] m_threadIDs;
    }

    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_notEmpty);
}

void ThreadPool::addTask(Task task) {
    if (m_shutdown) {
        return;
    }
    pthread_mutex_lock(&m_lock);
    m_taskQ->addTask(task);
    pthread_mutex_unlock(&m_lock);

    pthread_cond_signal(&m_notEmpty);
}

void *ThreadPool::worker(void *arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg); // 把void类型的指针强转成ThreadPool指针

    while (true) {
        pthread_mutex_lock(&pool->m_lock);
        // 任务队列为空，阻塞线程
        // 为什么进不来啊？
        while (0 == pool->m_taskQ->getQueueSize() && !pool->m_shutdown) {
            #if DEBUG_LEVEL
                std::cout << "thread " << std::to_string(pthread_self()) << " waiting..." << std::endl;
            #endif
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

            if (pool->m_exitNum > 0) {
                pool->m_exitNum --;
                if (pool->m_aliveNum > pool->m_minNum) {
                    pool->m_aliveNum --;
                    pthread_mutex_unlock(&pool->m_lock);
                    pool->threadExit();
                }
            }
        }

        // 线程池被关闭
        if (pool->m_shutdown) {
            pthread_mutex_unlock(&pool->m_lock);
            pool->threadExit();
        }

        // 正常工作，调用任务中的function，这里有锁
        Task task = pool->m_taskQ->getTask();
        pool->m_busyNum ++;
        pthread_mutex_unlock(&pool->m_lock);

        #if DEBUG_LEVEL
            std::cout << "thread " << std::to_string(pthread_self()) << " start working..." << std::endl;
        #endif
        task.function(task.arg);
        delete task.arg;
        task.arg = nullptr;

        #if DEBUG_LEVEL
            std::cout << "thread " << std::to_string(pthread_self()) << " ending..." << std::endl;
        #endif
        pthread_mutex_lock(&pool->m_lock);
        pool->m_busyNum --;
        pthread_mutex_unlock(&pool->m_lock);
    }

    return nullptr;
}

void *ThreadPool::manager(void *arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg); // 把void类型的指针强转成ThreadPool指针
// 如果线程池没有关闭, 就一直检测
    while (!pool->m_shutdown) {
        // 每隔5s检测一次
        sleep(5);
        // 取出线程池中的任务数和线程数量
        //  取出工作的线程池数量
        pthread_mutex_lock(&pool->m_lock);
        int queueSize = pool->m_taskQ->getQueueSize();
        int liveNum = pool->m_aliveNum;
        int busyNum = pool->m_busyNum;
        int maxNum = pool->m_maxNum;
        int minNum = pool->m_minNum;
        pthread_mutex_unlock(&pool->m_lock);

        // 创建线程
        const int NUMBER = 2;
        // 当前任务个数>存活的线程数 && 存活的线程数<最大线程个数
        if (queueSize > liveNum && liveNum < maxNum) {
            // 线程池加锁
            pthread_mutex_lock(&pool->m_lock);
            int num = 0;
            for (int i = 0; i < pool->m_maxNum && num < NUMBER
                            && pool->m_aliveNum < pool->m_maxNum; i ++) {
                if (pool->m_threadIDs[i] == 0) {
                    num ++;
                    pool->m_aliveNum ++;
                    #if DEBUG_LEVEL
                        std::cout << "###" << pool->m_aliveNum << ": child thread ID -> " << std::to_string(pool->m_threadIDs[i]) << std::endl;
                    #endif
                    pthread_create(&pool->m_threadIDs[i], nullptr, worker, pool);
                }
            }
            pthread_mutex_unlock(&pool->m_lock);
        }

        // 销毁多余的线程
        // 忙线程*2 < 存活的线程数目 && 存活的线程数 > 最小线程数量
        if (busyNum * 2 < liveNum && liveNum > minNum) {
            pthread_mutex_lock(&pool->m_lock);
            pool->m_exitNum = NUMBER;
            pthread_mutex_unlock(&pool->m_lock);
            for (int i = 0; i < NUMBER; i ++) {
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }
    }
    return nullptr;
}


void ThreadPool::threadExit() {
    pthread_t tid = pthread_self();

    for (int i = 0; i < m_maxNum; i ++) {
        if (m_threadIDs[i] == tid) {
            #if DEBUG_LEVEL
                std::cout << "threadExit() function, thread ID: "
                    << std::to_string(pthread_self()) << " exiting..." << std::endl;
            #endif
            m_threadIDs[i] = 0;
            break;
        }
    }

    pthread_exit(nullptr);
}