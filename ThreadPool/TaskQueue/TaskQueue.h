//
// Created by LesPaul on 9/9/2024.
//

#ifndef THREAD_POOL_TASKQUEUE_H
#define THREAD_POOL_TASKQUEUE_H

#include <queue>

using callback = void (*)(void *);

struct Task {
    Task() {}
    Task(callback func, void *x) :
        function(func), arg(x) {}

    callback function;
    void *arg;
};

class TaskQueue {
public:
    TaskQueue();
    ~TaskQueue();

    void addTask(Task task);
    Task getTask();
    inline int getQueueSize() {
        return m_taskQueue.size();
    }

private:
    std::queue<Task> m_taskQueue;
};


#endif //THREAD_POOL_TASKQUEUE_H
