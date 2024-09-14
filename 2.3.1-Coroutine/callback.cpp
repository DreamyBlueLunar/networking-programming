// 这种就是回调函数的效果，在另一个线程里面把事情做完，但是主线程还是可以继续走

#include <iostream>
#include <thread>

using cb = void (*)(int);

void callback(int x) {
    std::cout << "now in callback(), threadID: " 
        << std::this_thread::get_id() << " and x = "
        << x << "." << std::endl;
}

void func(cb print, int x) {
    std::cout << "now in func(), thread ID: " 
        << std::this_thread::get_id() << "." << std::endl;
    print(x);
}

int main(void) {
    std::cout << "now in main(), thread ID: "
        << std::this_thread::get_id() << "." << std::endl;
    
    std::thread tfunc(func, callback, 12138);

    std::cout << "now at the end of the program" << std::endl;
    tfunc.join();
    return 0;
}
