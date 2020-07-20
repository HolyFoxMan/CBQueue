#include "cbqadapter.hpp"

    typedef union {
        int a;
        float b;
        char c;
    } x_t;


int main(void)
{
    void printX(int size, x_t* x);

    printX(5, (x_t[5]) {{.b = 1.2f}, {.b = 2.3f}, {.b = 3.4f}, {.b = 4.5f}, {.b = 5.6f}});
    system("pause");
    return 0;
}

void printX(int size, x_t* x)
{
    for (int i = 0; i < size; i++)
        printf("%f", x[i].b);
}
