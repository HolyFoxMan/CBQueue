#include <stdio.h>

/* there should be a couple of such headers */
#include "cbqbuildconf.h"
#include "cbqueue.h"

/* QCallback type func */
int HelloWorld(UNUSED int argc, CBQArg_t* argv)
{
    printf("Hello, World!\n");
}

int main(void)
{
    CBQueue_t queue;

    /* initialize queue with tiny capacity (by define constant)
     * and static mode, without limit capacity
     * also with limit standard argument capacity
     */
    CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0);

    /* push callback into queue without arguments */
    CBQ_PushVoid(&queue, HelloWorld);

    /* run callback without catching return integer */
    CBQ_Exec(&queue, NULL); // Hello, World!

    CBQ_QueueFree(&queue);
    return 0;
}
