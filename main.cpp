/* C++ Debug Test Entry point */

#include <iostream>
//#include "cbqueue.h"
#include "cbqwrapper.hpp"

int CBTest(int, CBQArg_t* argv)
{
    std::cout << argv[0].sVar;
    return 0;
}

#include <vector>

int main(void)
{
    const char x[] = "Hello, World!";

    CBQPP::Queue queue;
    queue.Push(CBTest, x);
    queue.Execute();

    return 0;
}
