#include "ThreadPool/ThreadPool.h"
#include <iostream>
#include <unistd.h>

void printMsg(void *arg) {
    std::cout << "tid: " << pthread_self() << "." << std::endl;
}

int main() {
    std::cout << "A test for self-implement thread pool" << std::endl;

    ThreadPool pool = ThreadPool(4, 10);
    for (int i = 0; i < 100; i ++) {
        pool.addTask({printMsg, nullptr});
    }
    sleep(5);

    return 0;
}
