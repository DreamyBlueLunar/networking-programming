// 尝试lambda表达式
// 感觉写一点简单的函数还是挺好用的

#include <iostream>

using std::cout;
using std::endl;

int main(void) {
	int num1 = [](int x) {
		return x * x;
	}(2);

	cout << "num1 = " << num1 << endl;

	auto getNum = [](int x) {
		return x << 1;
	}(3);
	
	int num2 = getNum;
	cout << "num2 = " << num2 << endl;
	
	return 0;
}
