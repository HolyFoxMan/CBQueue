/* C++ Debug Test Entry point */

#include <iostream>
#include "cbqwrapper.hpp"

int HelloWorld(void)
{
    std::cout << "Hello, world!" << std::endl;
    return 0;
}

int DrawVars(int32_t a, char b, float c, double d)
{
    std::cout << a << " " << b << " " << c << " " << d << std::endl;
    return 0;
}

int Old_TestCB_1(int, CBQArg_t*)
{
    std::cout << "Test 1" << std::endl;
    return 0;
}


int Old_TestCB_2(int argc, CBQArg_t* argv)
{
    std::cout << "Test 2 " << argc << " " << argv[0].iVar << std::endl;
    return 0;
}

int main(void)
{
    CBQPP::Queue queue;

    queue.Push(HelloWorld);
    queue.Push(DrawVars, 3, 'a', 0.15f, 56.64);

    queue.Push( // calc
            [](int a, int b, char op) -> int {

                switch (op) {
                case '+':
                    return a + b;
                case '-':
                    return a - b;
                case '*':
                    return a * b;
                case '/':
                    return a / b;
                default:
                    return 0;
                };
            }
        , 6, 2, '-');

    queue.Push([](){std::cout << "Hi" << std::endl; return 0;});

    queue.Execute(); // Hello, World!
    queue.Execute(); // 3 a 0.15 56.64

    int res = 0;
    queue.Execute(&res);
    std::cout << res << std::endl;   // 4

    queue.Execute(); // Hi

    // Old push variant
    queue.Push(Old_TestCB_1);
    queue.Push(Old_TestCB_2, 5);

    queue.Execute(); // Test 1
    queue.Execute(); // Test 2 1 5

    queue.SetTimeout(Old_TestCB_2, 0, 4);
    queue.SetTimeout([](int a, char b){std::cout << "Test custom cb" << a << b << std::endl; return 0;}, 0, 5, 'v');
    queue.Execute();
    queue.Execute();

    return 0;
}
