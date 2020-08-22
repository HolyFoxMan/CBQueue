#include <iostream>

#include "cbqwrapper.h"

int Sum(int a, int b, int c)
{
    std::cout << a + b + c << std::endl;

    return 0;
}

int main()
{
    CBQPP::Queue queue;

    queue.Push(Sum, 1, 2, 3);

    queue.Execute();
}
