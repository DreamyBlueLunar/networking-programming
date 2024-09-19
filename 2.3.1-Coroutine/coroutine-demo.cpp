//
// Created by LesPaul on 9/11/2024.
//
#include <iostream>
#include <coroutine>

struct promise {
    struct promise_type {
        promise get_return_object() {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };

    std::coroutine_handle<promise_type> _h; // 表示协程本身？
};

struct Input {
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<> h) {}
    void await_resume() {}
};

promise func(void) {
    Input t;
    std::cout << "1" << std::endl;
    co_await t;
    std::cout << "2" << std::endl;
}

int main(void) {
//    auto lambda = []()->promise {
//        Input t;
//        std::cout << "1" << std::endl;
//        co_await t;
//        std::cout << "2" << std::endl;
//    };

//    promise res = lambda();
    promise res = func();
    std::cout << "main function" << std::endl;
    res._h.resume();

    return 0;
}
