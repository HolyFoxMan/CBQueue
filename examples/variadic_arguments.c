#include <stdio.h>

#include "cbqbuildconf.h"
#include "cbqueue.h"

// int a, int b, int c
int PrintSum(int argc, CBQArg_t* argv)
{
    if (argc == 0)
        return -1;

    int sum = 0;

    for (int i = 0; i < argc; ++i)
        sum += argv[0].fVar;
    printf("%d\n", sum);

    return 0;
}

int main()
{
    CBQueue_t queue;
    CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0);

    // 1 method
    /* see static_arguments.c to understand */
    CBQ_PushStatic(&queue, PrintSum, 3, (CBQArg_t){.iVar = 1}, (CBQArg_t){.iVar = 2}, (CBQArg_t){.iVar = 3});

    // 2 method
    CBQArg_t[] variadic_arg = {
        {.iVar = 7}, {.iVar = 8}, {.iVar = 4}, {.iVar = 6}, {.iVar = 5}
    };

    CBQ_PushOnlyVP(&queue, PrintSum, 5, variadic_arg); // 30

    CBQ_Exec(&queue, NULL); // 6

    CBQ_QueueFree(&queue);
    return 0;
}
