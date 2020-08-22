#include <stdio.h>

#include "cbqbuildconf.h"
#include "cbqueue.h"

// int a, int b, int c
int PrintSum(UNUSED int argc, CBQArg_t* argv)
{
    printf("%d\n", argv[0].iVar + argv[1].iVar + argv[2].iVar);
    return 0;
}

int main()
{
    CBQueue_t queue;
    CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0); // check hello_world.c for understand

    /* Push CB with 3 integers args */
    CBQ_PushStatic(&queue, PrintSum, 3, (CBQArg_t){.iVar = 1}, (CBQArg_t){.iVar = 2}, (CBQArg_t){.iVar = 3});

    CBQ_Exec(&queue, NULL); // 6

    CBQ_QueueFree(&queue);
    return 0;
}
