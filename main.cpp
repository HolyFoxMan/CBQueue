/* C++ Debug Test Entry point */

#include <iostream>
#include "cbqwrapper.hpp"

int CPPCBFunc(int a, char b, float c, double d)
{
    std::cout << a << " " << b << " " << c << " " << d << std::endl;
    return 0;
}

int CPPCBFunc2(void)
{
    std::cout << "Hello, world!" << std::endl;
    return 0;
}

int CPPCBFunc3(char a, char b)
{
    std::cout << a << " " << b << std::endl;
    return 0;
}

#include <vector>

int main(void)
{
    CBQPP::Queue queue;
    //queue.Push(CPPCBFunc, 3, 'a', 0.15, 56.64);
    queue.Push(CPPCBFunc3, 'a', 'b');
    queue.Execute();
    return 0;
}
