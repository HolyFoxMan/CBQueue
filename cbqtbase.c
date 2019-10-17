#include "cbqtest.h"

/* ---------------- Hello World ---------------- */
int funcHW(int argc, CBQArg_t* argv)
{
    printf("CB 1: Hello, world!\n");
    return 0;
}

int funcHU(int argc, CBQArg_t* argv)
{
    printf("CB 2: Hello, %s! Your age is %d.\n", argv[0].sVar, argv[1].iVar);
    free(argv[0].sVar);
    return 0;
}

int add(int argc, CBQArg_t* argv)
{
    int res = 0;

    while (argc > 0) {
        argc--;
        res += argv[argc].iVar;
    }
    printf("CB 3: Add result is %d.\n", res);

    return 0;
}


void CBQ_T_HelloWorld(void)
{
    CBQueue_t queue;
    const char username [] = "User";
    int age = 20;

    /* Print debug status */
    CBQ_OUTDEBUGSTATUS();

    /* Queue initialize */
    ASRT(CBQ_QueueInit(&queue, 3, CBQ_SM_STATIC, 0), "");

    /* Push hello world function into queue */
    ASRT(CBQ_Push(&queue, funcHW, 0, CBQ_NO_ARGS ), "");

    /* Push hello user function into queue */
    ASRT(CBQ_Push(&queue, funcHU, 2, (CBQArg_t) {.sVar = CBQ_strIntoHeap(username)}, (CBQArg_t) {.iVar = age} ), "");

    /* Push summ calc function */
    ASRT(CBQ_Push(&queue, add, 3, (CBQArg_t) {.iVar = 1}, (CBQArg_t) {.iVar = 2}, (CBQArg_t) {.iVar = 4}), "");

    if (CBQ_HAVECALL(queue))
        printf("main: calls num in queue: %llu\n", CBQ_GetCallAmount(&queue));

    /* Execute first pushed function */
    ASRT(CBQ_Exec(&queue, 0), "");

    /* Execute second pushed function */
    ASRT(CBQ_Exec(&queue, 0), "");

    /* Execute third pushed function */
    ASRT(CBQ_Exec(&queue, 0), "");

    if (!CBQ_HAVECALL(queue))
        printf("main: queue is empty\n");

    /* Queue free */
    ASRT(CBQ_QueueFree(&queue), "");
}

/* ---------------- Control Test ---------------- */

int counter(int argc, CBQArg_t* argv)
{
    static int count = 0;
    count++;
    printf("CB exec count is %d\n", count);
    return 0;
}

#if defined(_INC_CONIO) || defined(CONIO_H)
void CBQ_T_ControlTest(void)
{
    int quit = 0,
        key;
    size_t customSize,
        qSize,
        qEngagedSize;
  //      resultByteSize;
//    unsigned char* saveStateBuffer = NULL;
    CBQueue_t queue;

    CBQ_OUTDEBUGSTATUS();
    ASRT(CBQ_QueueInit(&queue, 16, CBQ_SM_MAX, 0), "Failed to init");

    quit = 0;
    printf("p - push, e - pop, c - change size, i - increment size, d - decrement size, q - exit.\n");
    do {
        if (kbhit())
            key = getch();
        else
            continue;

        if (key == 'P' || key == 'p' || key == 'E' || key == 'e' || key == 'Q' || key == 'q' ||
            key == 'C' || key == 'c' || key == 'I' || key == 'i' || key == 'D' || key == 'd' ||
            key == 'S' || key == 's' || key == 'L' || key == 'l') {

            system("cls");

            switch(key) {

                case 'P':
                case 'p': {
                    ASRT(CBQ_Push(&queue, counter, 0, CBQ_NO_ARGS), "Failed to push");
                    break;
                }

                case 'E':
                case 'e': {
                    ASRT(CBQ_Exec(&queue, NULL), "Failed to pop");
                    break;
                }

                case 'Q':
                case 'q': {
                    quit = 1;
                    break;
                }

                case 'C':
                case 'c': {
                    printf("Type new size\n");
                    scanf(SZ_PRTF, &customSize);
                    fflush(stdin);
                    ASRT(CBQ_ChangeSize(&queue, 0, customSize), "Failed to change size");
                    break;
                }

                case 'I':
                case 'i': {
                    ASRT(CBQ_ChangeSize(&queue, CBQ_INC_SIZE, 0), "Failed to increment size");
                    break;
                }

                case 'D':
                case 'd': {
                    ASRT(CBQ_ChangeSize(&queue, CBQ_DEC_SIZE, 0), "Failed to decrement size");
                    break;
                }
/*
                case 'S':
                case 's': {
                    ASRT(CBQ_SaveState(&queue, saveStateBuffer, &resultByteSize), "Saving data error");
                    printf("Received size (in bytes): %llu\n", resultByteSize);
                    break;
                }

                case 'L':
                case 'l': {
                    ASRT(CBQ_RestoreState(&queue, saveStateBuffer, resultByteSize), "Loading data error");
                    break;
                }
                */
            }

            ASRT(CBQ_GetFullInfo(&queue, NULL, &qSize, &qEngagedSize, NULL, NULL), "");
            printf("Size: " SZ_PRTF ", engaged size: "
            SZ_PRTF "\n", qSize, qEngagedSize);
        }
    } while(!quit);

    ASRT(CBQ_QueueFree(&queue), "Failed to free");
}
#else
void CBQ_T_ControlTest(void)
{
    printf("conio lib is not supported for control test\n");
}
#endif

/* ---------------- SizeModes Test ---------------- */

int occupyAllCells(CBQueue_t* queue)
{
    size_t size, engSize, i;
    int errSt = 0;

    CBQ_GetFullInfo(queue, NULL, &size, &engSize, NULL, NULL);
    size -= engSize;    // last empty cells

    for (i = 0; i < size; i++) {
        errSt = CBQ_Push(queue, counter, 0, CBQ_NO_ARGS);
        if (errSt)
            break;
    }

    return errSt;
}

int execAllCells(CBQueue_t* queue)
{
    int errSt = 0;

    while(CBQ_HAVECALL_P(queue)) {
        errSt = CBQ_Exec(queue, NULL);
        if (errSt)
            break;
    }

    return errSt;
}

int occupyCustomCells(CBQueue_t* queue, size_t num)
{
    int errSt = 0;

    while(num--) {
        errSt = CBQ_Push(queue, counter, 0, CBQ_NO_ARGS);
        if (errSt)
            break;
    }

    return errSt;
}

int execCustomCells(CBQueue_t* queue, size_t num)
{
    int errSt = 0;

    while(num--) {
        errSt = CBQ_Exec(queue, NULL);
        if (errSt)
            break;
    }

    return errSt;
}

int toState_3(CBQueue_t* queue)
{
    int errSt;

    errSt = occupyAllCells(queue);
    if (errSt)
        return errSt;
    /* ABCDEFGHIKLMNOPQ
     * b...............
     */

    errSt = execCustomCells(queue, CBQ_SI_TINY / 2);
    if (errSt)
        return errSt;
    /* --------IKLMNOPQ
     * s.......r.......
     */

     return 0;
}

int toState_4(CBQueue_t* queue)
{
    int errSt;

    errSt = occupyAllCells(queue);
    if (errSt)
        return errSt;
    /* ABCDEFGHIKLMNOPQ
     * b...............
     */

    errSt = execCustomCells(queue, CBQ_SI_TINY / 2);
    if (errSt)
        return errSt;
    /* --------IKLMNOPQ
     * s.......r.......
     */

    errSt = occupyCustomCells(queue, CBQ_SI_TINY / 4);
    if (errSt)
        return errSt;
    /* RSTU----IKLMNOPQ
     * ....s...r.......
     */

    return 0;
}

void CBQ_T_SizemodeTest(void)
{
    /* STATIC test */
    CBQueue_t queue;

    ASRT(CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0), "Init failed");
    /* ----------------
     * b...............
     */

     ASRT(toState_3(&queue), "Failed set to state 3");

     ASRT(CBQ_QueueFree(&queue), "Failed to free");

}

int selfExecCB(int argc, CBQArg_t* argv)
{
    static int part = 0;
    int sterr;

    printf("CB: Self queue executing Number %d\n", part);
    fflush(stdout);

    part++;

    sterr = CBQ_Exec(argv[0].qVar, 0);

    if (sterr)
        return sterr;

    part--;
    return 0;
}

void CBQ_T_BusyTest(void)
{
    int errSt = 0;
    CBQueue_t queue;

    ASRT(CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0), "Init failed");

    ASRT(CBQ_Push(&queue, selfExecCB, 1, (CBQArg_t) {.qVar = &queue}), "Push error")

    CBQ_Exec(&queue, &errSt);
    if (errSt == CBQ_ERR_IS_BUSY)
        printf("Error of self exec was successful handled\n");

    ASRT(CBQ_QueueFree(&queue), "Failed to free");
}
