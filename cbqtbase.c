#include "cbqtest.h"

/* ---------------- Hello World ---------------- */
int funcHW(UNUSED int argc, UNUSED CBQArg_t* argv)
{
    printf("CB HW: Hello, world!\n");
    return 0;
}

int funcHU(UNUSED int argc, CBQArg_t* argv)
{
    printf("CB age: Hello, %s! Your age is %d.\n", argv[0].sVar, argv[1].iVar);
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
    printf("CB sum: Add result is %d.\n", res);

    return 0;
}


void CBQ_T_HelloWorld(void)
{
    CBQueue_t queue;
    size_t capacity;
    const char username [] = "User";
    int age = 20;

    /* Print debug status */
    CBQ_OUTDEBUGSTATUS();

    /* Queue initialize */
    ASRT(CBQ_QueueInit(&queue, 3, CBQ_SM_STATIC, 0, 0), "")

    /* Push hello world function into queue */
    ASRT(CBQ_Push(&queue, funcHW, 0, NULL, 0, CBQ_NO_STPARAMS), "")

    /* Push hello user function into queue */
    ASRT(CBQ_Push(&queue, funcHU, 0, NULL, 2, (CBQArg_t) {.sVar = CBQ_strIntoHeap(username)}, (CBQArg_t) {.iVar = age} ), "")

    /* Push summ calc function */
    ASRT(CBQ_Push(&queue, add, 0, NULL, 3, (CBQArg_t) {.iVar = 1}, (CBQArg_t) {.iVar = 2}, (CBQArg_t) {.iVar = 4}), "")

    if (CBQ_HAVECALL(queue)) {
        ASRT(CBQ_GetSize(&queue, &capacity),"")
        printf("main: calls num in queue: %llu\n", capacity);
    }

    /* Execute first pushed function */
    ASRT(CBQ_Exec(&queue, 0), "")

    /* Execute second pushed function */
    ASRT(CBQ_Exec(&queue, 0), "")

    /* Execute third pushed function */
    ASRT(CBQ_Exec(&queue, 0), "")

    if (!CBQ_HAVECALL(queue))
        printf("main: queue is empty\n");

    /* Queue free */
    ASRT(CBQ_QueueFree(&queue), "")
}

/* ---------------- Control Test ---------------- */

int counterCB(int argc, CBQArg_t* argv)
{
    static int count = 0;
    count++;
    if (argc) {
        printf("CB: func counter is %d, arg cointer: %d\n", count, argv[0].iVar);
    if (argv[0].iVar != count)
        printf("CB: Warning! Counter value mismatch\n");
    } else
        printf("CB: func counter is %d\n", count);
    fflush(stdout);
    return 0;
}

int counterPusherCB(UNUSED int argc, CBQArg_t* argv)
{
    ASRT(CBQ_PushStatic(argv[0].qVar, counterCB, 1, (CBQArg_t) {.iVar = argv[1].iVar}), "Failed to push counter cb")
    return 0;
}

#define ALPH_CAPACITY 26

/*
void CBQ_drawArgpAsChars__(CBQueue_t* trustedQueue)
{
    struct CBQContainer_t* co_r;
    for (size_t i = 0; i < trustedQueue->capacity; i++) {
        co_r = *trustedQueue->coArr[i];
        printf("%c", co_r->args % ALPH_CAPACITY + 'a');
    }
    printf("\n");
} */

int fillQueueCB(UNUSED int argc, CBQArg_t* args)
{
    do {
        ASRT(CBQ_PushN(args[0].qVar, add, {1}, {2}, {3}), "Failed to push in CB")
    } while (!CBQ_ISFULL_P(args[0].qVar));
        ASRT(CBQ_PushN(args[0].qVar, add, {1}, {2}, {3}), "Failed to push in CB")
    return 0;
}

#if defined(_INC_CONIO) || defined(CONIO_H)
void CBQ_T_ControlTest(void)
{
    int quit = 0,
        key,
        inCB = 0,
        qStatus,
        errSt = 0;
    size_t customCapacity,
        qCapacity,
        qEngagedSize,
        qCapacityBytes;
    static int counter = 0;

//      resultByteCapacity;
//  unsigned char* saveStateBuffer = NULL;
    CBQueue_t queue;

    CBQ_OUTDEBUGSTATUS();
    ASRT(CBQ_QueueInit(&queue, 16, CBQ_SM_MAX, 0, 0), "Failed to init")
    quit = 0;
    printf("p - push, e - pop, c - change capacity, i - increment capacity, d - decrement capacity, q - exit.\n");
    do {

        if (kbhit()) {
            key = getch();
            switch(key) {
            case 'P': case 'p':
            case 'C': case 'c':
            case 'E': case 'e':
            case 'Q': case 'q':
            case 'I': case 'i':
            case 'F': case 'f':
            case 'D': case 'd':
            case 'N': case 'n':
            case 'M': case 'm':
            // case 'S': case 's':
            // case 'L': case 'l':
                system("cls");
                break;
            default:
                continue;
            }
        } else
            continue;

        switch(key) {

            case 'P':
            case 'p': {
                counter++;
                if (!inCB)
                    ASRT(errSt = CBQ_PushStatic(&queue, counterCB, 1, (CBQArg_t) {.iVar = counter}), "Failed to push")
                else
                    ASRT(errSt = CBQ_PushStatic(&queue, counterPusherCB, 2, (CBQArg_t) {.qVar = &queue}, (CBQArg_t) {.iVar = counter}), "Failed to push")
                    if (errSt)
                        counter--;
                break;
            }

            case 'E':
            case 'e': {
                ASRT(CBQ_Exec(&queue, NULL), "Failed to pop")
                break;
            }

            case 'Q':
            case 'q': {
                quit = 1;
                break;
            }

            case 'C':
            case 'c': {
                printf("Type new capacity\n");
                scanf(SZ_PRTF, &customCapacity);
                fflush(stdin);
                ASRT(CBQ_ChangeCapacity(&queue, 0, customCapacity, 0), "Failed to change capacity")
                break;
            }

            case 'I':
            case 'i': {
                ASRT(CBQ_ChangeCapacity(&queue, CBQ_INC_CAPACITY, 0, 1), "Failed to increment capacity")
                break;
            }

            case 'D':
            case 'd': {
                ASRT(CBQ_ChangeCapacity(&queue, CBQ_DEC_CAPACITY, 0, 1), "Failed to decrement capacity")
                break;
            }
#ifdef CBQD_SCHEME
            case 'F':
            case 'f': {
                inCB = !inCB;
                ASRT(CBQ_DRAWSCHEME(&queue),"")
                break;
            }
#endif
            case 'N':
            case 'n': {
                ASRT(CBQ_PushN(&queue, fillQueueCB, {.qVar = &queue}), "")
                break;
            }

            case 'M':
            case 'm': {
                int nISM, tryToAdaptCapacity = 0, adaptSML = 0;
                size_t nSML;

                printf("Select new capacity Mode:\n%d - static\n%d - limit\n%d - max capacity\n9 - cancel\n",
                       CBQ_SM_STATIC, CBQ_SM_LIMIT, CBQ_SM_MAX);
                do {
                    scanf("%d", &nISM);
                    fflush(stdin);
                    if (nISM == CBQ_SM_STATIC || nISM == CBQ_SM_LIMIT || nISM == CBQ_SM_MAX || nISM == 9)
                        break;
                    else
                        printf("Wrong value\n");
                } while(0);

                if (nISM == 9)
                    break;

                if (nISM == CBQ_SM_LIMIT) {
                    printf("Type new limit capacity:\n");
                    scanf(SZ_PRTF, &nSML);
                    fflush(stdin);

                    printf("Change the capacity of the queue, if it does not fit? (1/0)");
                    scanf("%d", &key);
                    fflush(stdin);
                    if (key == 1)
                        tryToAdaptCapacity = 1;

                    printf("Align max capacity limit, if it affects busy cells? (1/0)");
                    scanf("%d", &key);
                    fflush(stdin);
                    if (key == 1)
                        adaptSML = 1;
                }

                ASRT(CBQ_ChangeIncCapacityMode(&queue, nISM, nSML, tryToAdaptCapacity, adaptSML), "")
                break;
            }
/*
            case 'S':
            case 's': {
                ASRT(CBQ_SaveState(&queue, saveStateBuffer, &resultByteCapacity), "Saving data error")
                printf("Received capacity (in bytes): %llu\n", resultByteCapacity);
                break;
            }

            case 'L':
            case 'l': {
                ASRT(CBQ_RestoreState(&queue, saveStateBuffer, resultByteCapacity), "Loading data error")
                break;
            }
            */
        }

        ASRT(CBQ_GetFullInfo(&queue, &qStatus, &qCapacity, &qEngagedSize, NULL, NULL, &qCapacityBytes), "")

        printf("Status: %d, capacity: " SZ_PRTF ", engaged capacity: "
        SZ_PRTF " in bytes: " SZ_PRTF " run in CB: %s\n", qStatus, qCapacity, qEngagedSize, qCapacityBytes, inCB? "true" : "false");
        // drawArgpAsChars(&queue);

    } while(!quit);

    ASRT(CBQ_QueueFree(&queue), "Failed to free")
}
#else
void CBQ_T_ControlTest(void)
{
    printf("conio lib is not supported for control test\n");
}
#endif

/* ---------------- CapacityModes Test ---------------- */

int occupyAllCells(CBQueue_t* queue)
{
    size_t capacity, engSize, i;
    int errSt = 0;

    CBQ_GetFullInfo(queue, NULL, &capacity, &engSize, NULL, NULL, NULL);
    capacity -= engSize;    // last empty cells

    for (i = 0; i < capacity; i++) {
        errSt = CBQ_Push(queue, counterCB, 0, NULL, 0, CBQ_NO_STPARAMS);
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
        errSt = CBQ_Push(queue, counterCB, 0, NULL, 0, CBQ_NO_STPARAMS);
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

void CBQ_T_CapacitymodeTest(void)
{
    /* STATIC test */
    CBQueue_t queue;

    ASRT(CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0), "Init failed")
    /* ----------------
     * b...............
     */

     ASRT(toState_3(&queue), "Failed set to state 3")

     ASRT(CBQ_QueueFree(&queue), "Failed to free")

}

int selfExecCB(UNUSED int argc, CBQArg_t* argv)
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

int changeCapacityCB(UNUSED int argc, CBQArg_t* argv)
{
    return CBQ_ChangeCapacity(argv[0].qVar, argv[1].iVar, argv[2].szVar, 1);
}


int freeQueueCB(UNUSED int argc, CBQArg_t* argv)
{
    return CBQ_QueueFree(argv[0].qVar);
}

void CBQ_T_BusyTest(void)
{
    int errSt = 0;
    CBQueue_t queue;

    ASRT(CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0), "Init failed")

    ASRT(CBQ_Push(&queue, selfExecCB, 0, NULL, 1, (CBQArg_t) {.qVar = &queue}), "Push error")

    CBQ_Exec(&queue, &errSt);
    if (errSt == CBQ_ERR_IS_BUSY)
        printf("Error of intermdeiary CB exec was successful handled\n");

    ASRT(CBQ_PushStatic(&queue, changeCapacityCB, 3,
        (CBQArg_t) {.qVar = &queue},
        (CBQArg_t) {.iVar = CBQ_DEC_CAPACITY},
        (CBQArg_t) {.szVar = 0}
    ),"Failed to push CB for custom changing capacity")

    CBQ_Exec(&queue, &errSt);
    if (errSt == CBQ_ERR_IS_BUSY)
        printf("Error of changing capacity through an intermdeiary was successful handled\n");

    ASRT(CBQ_PushStatic(&queue, freeQueueCB, 1,
        (CBQArg_t) {.qVar = &queue}
    ),"Failed to push CB for custom changing capacity")

    CBQ_Exec(&queue, &errSt);
    if (errSt == CBQ_ERR_IS_BUSY)
        printf("Error of free queue through an intermdeiary was successful handled\n");

    ASRT(CBQ_QueueFree(&queue), "Failed to free")
}

/* sum of ints */
int addAllNumsCB(int argc, CBQArg_t* args)
{
    int i;
    int sum;

    for(i = 0, sum = 0; i < argc; i++)
        sum += args[i].iVar;

    printf("Arg count: %d\n", argc);
    printf("CB: The sum is %d\n", sum);

    return 0;
}

int mulAllNumsCB(int argc, CBQArg_t* args)
{
    int i;
    int pro;

    for(i = 0, pro = 1; i < argc; i++)
        pro *= args[i].iVar;

    printf("CB: The product is %d\n", pro);

    return 0;
}

int calcNumsCB(int argc, CBQArg_t* args)
{
    int i;

    printf("CB: arguments: ");
    for(i = 2; i < argc; i++)
        printf("%d ",args[i].iVar);
    printf("\n");

    switch(args[1].cVar) {
    case '+': {
        CBQ_PushOnlyVP(args[0].qVar, addAllNumsCB, argc - 2, args + 2);
        printf("Addition selected\n");
        break;
    }
    case '*': {
        CBQ_PushOnlyVP(args[0].qVar, mulAllNumsCB, argc - 2, args + 2);
        printf("Multiplication selected\n");
        break;
    }
    default: {
        printf("Error, unknown operation\n");
        break;
        }
    }

    return 0;
}

#define P_LINE(EXP) \
    printf("%s\n", MVAL_TO_STR(EXP))

void CBQ_T_Params(void)
{
    CBQueue_t queue;
    int numc = 4;
    CBQArg_t nums[4] = {
            {.iVar = 4},
            {.iVar = 7},
            {.iVar = 9},
            {.iVar = 15}
    };

    ASRT(CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0), "Failed to init")

    /* Variable params passing, sum: 35 */
    ASRT(CBQ_PushOnlyVP(&queue, addAllNumsCB, numc, nums),"Failed to push CB with variable params")

    /*  Static params passing, sum: 10 */
    ASRT(CBQ_PushStatic(&queue, addAllNumsCB, 2,
        (CBQArg_t) {.iVar = 4},
        (CBQArg_t) {.iVar = 6}),
    "Failed to push CB with static params")

    printf("Test of calc sum of 4, 7, 9 and 15 (35) by variable params\n");
    ASRT(CBQ_Exec(&queue, NULL), "Failed to exec with variable params")

    printf("Test of calc sum of 4 and 6 (10) by static params\n");
    ASRT(CBQ_Exec(&queue, NULL), "Failed to exec with static params")

    /* Now test with combine parameters - product of 4 nums (3780) */
    ASRT(CBQ_Push(&queue, calcNumsCB, numc, nums, 2,
        (CBQArg_t) {.qVar = &queue},
        (CBQArg_t) {.cVar = '*'}),
    "Error to push calc CB")

    printf("Test with combine parameters: multiplication\n");
    ASRT(CBQ_Exec(&queue, NULL), "Failed to exec with combine params")
    ASRT(CBQ_Exec(&queue, NULL), "Failed to exec with calculation")

    CBQ_PushN(&queue, addAllNumsCB, {1}, {2}, {3}, {4}); // 10
    // P_LINE(CBQ_PushN(&queue, addAllNumsCB, 1, 2, 3, 4));
    CBQ_Exec(&queue, NULL);

    ASRT(CBQ_QueueFree(&queue),"Failed to free")
}

/* base set timeout test */
void CBQ_T_SetTimeout(void)
{
    CBQueue_t queue;
    CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0);
    int retst = 0;

    /* Hello world after 2 sec */
    ASRT(CBQ_SetTimeoutVoidSP(&queue, 2, 1, funcHW), "error to set time")

    /* sum of 1, 2, 3 after 3 sec at start of execution */
    ASRT(CBQ_SetTimeoutSP(&queue, 3, 1, add, 3, (CBQArg_t[]) {{1}, {2}, {3}} ), "failed to push add cb")

    while(CBQ_HAVECALL(queue)) {
        ASRT(CBQ_Exec(&queue, &retst), "error to exec");
        if (retst)
            printf("Returned error code by set timeout: %d\n", retst);
    }

    CBQ_QueueFree(&queue);
}

#define POINTERS_MAX 15
#define F_W 30
#define F_H 20
#define ST_DEL 1

typedef struct {
    int x;
    int y;
    char c;
    int st;
} point_t;

typedef struct {
    point_t arr[POINTERS_MAX];
    int curActive;
} pointers_t;

int ptrLife(int argc, CBQArg_t* args)
{
    point_t* ptr = (point_t*) args[1].pVar;
    char (*field)[F_W] = (char(*)[F_W]) args[2].pVar;

    /* rand move */
    if (ptr->x == 0)
        ++ptr->x;
    else if (ptr->x == F_W - 1)
        --ptr->x;
    else
        ptr->x += rand() % 3 - 1;

    if (ptr->y == 0)
        ++ptr->y;
    else if (ptr->y == F_H - 1)
        --ptr->y;
    else
        ptr->y += rand() % 3 - 1;

    /* determinate pos in array */
    if (field[ptr->y][ptr->x] && ptr->c != field[ptr->y][ptr->x]) {

        ptr->st = 0;
        /* decrement cur active points */
        *((int*) args[3].pVar) -= 1;
        return 0;

    } else
        field[ptr->y][ptr->x] = ptr->c;

    return CBQ_SetTimeoutSP(args[0].qVar, ST_DEL + 1, 1, ptrLife, argc, args);

    return 0;
}

/* draw screen */
int drawScreenCB(int argc, CBQArg_t* args)
{
    char (*field)[F_W] = (char(*)[F_W]) args[1].pVar;

    system("clear");
    for (int i = 0; i < F_H; i++) {
        for (int j = 0; j < F_W; j++)
            printf("%c", field[i][j]);
        printf("\n");
    }
    printf("active points: %d\nexit - q\n", *((int*) args[2].pVar));
    fflush(stdout);

    return CBQ_SetTimeoutSP(args[0].qVar, ST_DEL, 1, drawScreenCB, argc, args); // 0 or err
}

void CBQ_T_SetTimeout_AutoGame(void)
{
    CBQueue_t queue;
    int rstat = 0;
    pointers_t ptrs = (pointers_t) {
        .arr = {
            {4, 6, '*', 1},
            {7, 3, '+', 1},
            {6, 10, '*', 1},
            {8, 5, '+', 1},
            {14, 1, '*', 1},
            {1, 5, '+', 1}
        },
        .curActive = 6
    };
    char field[F_H][F_W] = {0};

    ASRT(CBQ_QueueInit(&queue, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0), "Failed to init")

    ASRT(CBQ_PushStatic(&queue, drawScreenCB, 3,
            (CBQArg_t) {.qVar = &queue},
            (CBQArg_t) {.pVar = (void*) field},
            (CBQArg_t) {.pVar = (void*) &ptrs.curActive}),
        "Failed to push draw screen cb")

    for (int i = 0; i < ptrs.curActive; i++)
        ASRT(CBQ_PushStatic(&queue, ptrLife, 4,
            (CBQArg_t) {.qVar = &queue},
            (CBQArg_t) {.pVar = &ptrs.arr[i]},
            (CBQArg_t) {.pVar = (void*) field},
            (CBQArg_t) {.pVar = (void*) &ptrs.curActive}),
        "failed to start pointer life cycle")

    srand(time(NULL));

    for(;;) {
        if (kbhit() && getch() == 'q')
            break;

        if (CBQ_HAVECALL(queue))
            ASRT(rstat = CBQ_Exec(&queue, &rstat), "failed to exec")
        else
            break;

        if (rstat) {
            printf("callback returned err code %d\n", rstat);
            break;
        }
    }
    if (!rstat)
        printf("End of game");

    ASRT(CBQ_QueueFree(&queue),"Failed to free")
}

void CBQ_T_VerIdInfo(int APIVer)
{
    if (!CBQ_GetVerIndex()) {
        printf("Information of current build not generated (Use GEN_VERID macro for it)\n");
        return;
    }
    if (APIVer != CBQ_CheckVerIndexByFlag(CBQ_VI_VERSION))
        printf("Warning! Variance of the API (cbqueue.h) version with the library version.\n"
               "There may be problems using. API Version is \"%d\"\n", APIVer);
    printf("VerId: %d\n", CBQ_GetVerIndex());
    printf("Version: %d\n", CBQ_CheckVerIndexByFlag(CBQ_VI_VERSION));
    if (CBQ_IsCustomisedVersion())
        printf("This lib have custom configuration\n");
    else
        printf("This lib dont have custom configuration, its safe for use\n");
    printf("Base check status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_NBASECHECK)? "false" : "true");
    printf("Busy check status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_NEXCOFBUSY)? "false" : "true");
    printf("Rest mem after fail status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_NRESTMEMFAIL)? "false" : "true");
    printf("Fix arg types status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_NFIXARGTYPES)? "false" : "true");
    printf("VParam check status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_NVPARAMCHECK)? "false" : "true");
    printf("Register vars status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_REGCYCLEVARS)? "true" : "false");
    printf("Debug status: %s\n", CBQ_CheckVerIndexByFlag(CBQ_VI_DEBUG)? "true" : "false");
}

int CB_0_Args(int argc, UNUSED CBQArg_t* args)
{
    if (argc != 0)
        printf("CB: Error of 0 args\n");
    else
        printf("CB: Void func runs\n");
    return 0;
}

int CB_2_Args_Sum(int argc, CBQArg_t* args)
{
    if (argc == 2)
        printf("CB: sum result is %d\n", args[0].iVar + args[1].iVar);
    else
        printf("CB: err, is not 2 args");

    return 0;
}

int CB_5_Args_PrintNums(int argc, CBQArg_t* args)
{
    if (argc != 5) {
        printf("CB: err, is not 5 args");
        return -1;
    }
    printf("Cb: ");
    for (int i = 0; i < 5; i++)
        printf("%d ", args[i].iVar);
    printf("\n");

    return 0;
}

void CBQ_T_ArgsTest(void)
{
    CBQueue_t queue;
    printf("Test: Init with 2 args\n");
    CBQ_QueueInit(&queue, 3, CBQ_SM_LIMIT, CBQ_SI_SMALL, 2);

    ASRT(CBQ_PushVoid(&queue, CB_0_Args), "Push void args cb err")
    ASRT(CBQ_PushStatic(&queue, CB_2_Args_Sum, 2, (CBQArg_t){.iVar = 4}, (CBQArg_t){.iVar = 6}), "Cant push")

    ASRT(CBQ_EqualizeArgsCapByCustom(&queue, 5, 1), "Cant equalize args");
    ASRT(CBQ_ChangeInitArgsCapByCustom(&queue, 5), "Failed to init cap")     // change init args from 2 to 5

    ASRT(CBQ_PushN(&queue, CB_5_Args_PrintNums, {1}, {2}, {3}, {4}, {5}), "")   // queue capacity is auto inc in that part
    ASRT(CBQ_PushN(&queue, CB_5_Args_PrintNums, {5}, {4}, {3}, {2}, {1}), "")

    for (int i = 0; i < 4; i++)
        CBQ_Exec(&queue, NULL);

    CBQ_QueueFree(&queue);
}

void CBQ_T_CopyTest(void)
{
    CBQueue_t q1, q2;
    ASRT(CBQ_QueueInit(&q1, CBQ_SI_TINY, CBQ_SM_MAX, 0, 0), "queue create failed")

    for (int i = 0; i < CBQ_SI_TINY; i++) {
        ASRT(CBQ_PushN(&q1, mulAllNumsCB, {1}, {2}, {3}), "push failed")
    }

    ASRT(CBQ_QueueCopy(&q2, &q1), "queue copy failed")

    for (int i = 0; i < CBQ_SI_TINY; i++) {
        printf("Queue 1: ");
        ASRT(CBQ_Exec(&q1, NULL), "push failed")
        printf("Queue 2: ");
        ASRT(CBQ_Exec(&q2, NULL), "push failed")
    }

    CBQ_QueueFree(&q1);
    CBQ_QueueFree(&q2);
}

void CBQ_T_ConcatTest(void)
{
    CBQueue_t q1, q2;
    CBQ_QueueInit(&q1, CBQ_SI_TINY, CBQ_SM_LIMIT, 16, 0);
    CBQ_QueueInit(&q2, CBQ_SI_TINY, CBQ_SM_STATIC, 0, 0);

    for (int i = 0; i < CBQ_SI_TINY; i++) {
        CBQ_PushN(&q1, mulAllNumsCB, {i}, {i + 1});
        CBQ_PushN(&q2, mulAllNumsCB, {i}, {i + 1});
    }

    ASRT(CBQ_QueueConcat(&q1, &q2), "Failed to concat")

    for (int i = 0; i < 16; i++)
        ASRT(CBQ_Exec(&q1, NULL), "failed to exec")

    CBQ_QueueFree(&q1);
    CBQ_QueueFree(&q2);
}
