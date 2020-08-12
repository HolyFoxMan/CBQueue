/* C++ Debug Test Entry point */

#include <iostream>
#include "cbqwrapper.hpp"

int CBTest(int, CBQArg_t* argv)
{
    std::cout << argv[0].iVar;
    return 0;
}

#include <vector>

int main(void)
{
    uint8_t x = 34;

    CBQPP::Queue
        queue,
        queue_2;

    queue.SetTimeout(queue_2, CBTest, 0, x);

    queue.Execute();
    queue_2.Execute();

    return 0;
}
