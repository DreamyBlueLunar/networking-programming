//
// Created by LesPaul on 9/9/2024.
//

#include "TaskQueue.h"

TaskQueue::TaskQueue() {}

TaskQueue::~TaskQueue() {}

void TaskQueue::addTask(Task task) {
    m_taskQueue.push(task);
}

Task TaskQueue::getTask() {
    Task task;
    if (!m_taskQueue.empty()) {
        task = m_taskQueue.front();
        m_taskQueue.pop();
    }
    return task;
}