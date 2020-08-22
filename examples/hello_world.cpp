#include <iostream>

#include "cbqwrapper.h"

int main()
{
    CBQCPP::Queue queue;

    queue.Push(
        []() {
            std::cout << "Hello, World!" << std::endl;
            return 0;
        }
    );

    queue.Execute(); // Hello, World

    return 0;
}
