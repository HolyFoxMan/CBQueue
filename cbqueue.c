#include "cbqueue.h"
#include "cbqlocal.h"


/* ---------------- Base methods ---------------- */
int CBQ_QueueInit(CBQueue_t* queue, size_t size, int sizeMode, size_t sizeMaxLimit);
int CBQ_QueueFree(CBQueue_t* queue);

int CBQ_ChangeSize(CBQueue_t* queue, int changeTowards, size_t customNewSize);

/*
int CBQ_SaveState(CBQueue_t* queue, unsigned char* data, size_t* receivedSize);
int CBQ_RestoreState(CBQueue_t* queue, unsigned char* data, size_t size);
*/

/* ---------------- Container methods ---------------- */
int CBQ_containerInit__(CBQContainer_t* container);
int CBQ_reallocSizeToAccepted__(CBQueue_t* trustedQueue, size_t newSize);
int CBQ_offsetToBeginning__(CBQueue_t* trustedQueue);

    /* Incrementation */
    size_t CBQ_calcNewIncSize__(CBQueue_t* trustedQueue);
    int CBQ_incSizeCheck__(CBQueue_t* trustedQueue, size_t newSize);
    int CBQ_incSize__(CBQueue_t* trustedQueue, size_t newSize);

    /* Decrementation */
    int CBQ_decSize__(CBQueue_t* trustedQueue, size_t newSize);
    size_t CBQ_decSizeAlignment__(CBQueue_t* trustedQueue, size_t newSize);

/* ---------------- Container Args Methods ---------------- */
int CBQ_coIncMaxArgSize__(CBQContainer_t* container, size_t newSize);
#define CBQ_COARGFREE_P__(P_CONTAINER) \
    free(P_CONTAINER.args)

/* ---------------- Call Methods ---------------- */
int CBQ_Push(CBQueue_t* queue, QCallback func, int varParamc, CBQArg_t* varParams, int stParamc, CBQArg_t stParams, ...);
int CBQ_Exec(CBQueue_t* queue, int* funcRetSt);
int CBQ_Clear(CBQueue_t* queue);

/* ---------------- Info Methods ---------------- */
// CBQ_HAVECALL(TRUSTED_QUEUE);
// CBQ_GETSIZE(TRUSTED_QUEUE);
size_t CBQ_GetCallAmount(CBQueue_t* queue);
int CBQ_GetFullInfo(CBQueue_t* queue, int* getStatus, size_t* getSize, size_t* getEngagedSize,
    int* getSizeMode, size_t* getSizeMaxLimit);

/* ---------------- Other Methods ---------------- */
char* CBQ_strIntoHeap(const char* str);

/* inline methods by macros */
#ifdef CBQD_SCHEME

    /* void* type because thats declared in debug header,
     * which was included before CBQueue_t* type definition
     */
    int CBQ_drawScheme_chk__(void* queue);
    void CBQ_drawScheme__(CBQueue_t* trustedQueue);
    #define CBQ_DRAWSCHEME_IN(P_TRUSTED_QUEUE) \
        CBQ_drawScheme__(P_TRUSTED_QUEUE)

#else

    #define CBQ_DRAWSCHEME_IN(P_TRUSTED_QUEUE) \
        ((void)0)

#endif // CBQD_SCHEME

#ifdef CBQD_OUTPUTLOG

    #define CBQ_MSGPRINT(STR) \
        printf("Notice: %s\n", STR)

#else

    #define CBQ_MSGPRINT(STR) \
        ((void)0)

#endif // CBQD_OUTPUTLOG

////////////////////////////////////////////////////////////////////////////////


/* ---------------- Base methods ---------------- */
int CBQ_QueueInit(CBQueue_t* queue, size_t size, int sizeMode, size_t sizeMaxLimit)
{
    size_t i;
    CBQueue_t iniQueue = {
        .execSt = CBQ_EST_NO_EXEC,
        .size = size,
        .incSize = INIT_INC_SIZE,
        .rId = 0,
        .sId = 0,
        .status = CBQ_ST_EMPTY

        #ifdef CBQD_SCHEME
        , .curLetter = 'A'
        #endif // CBQD_SCHEME
    };

    /* base error checking */
    if (queue == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;

    if (queue->initSt == CBQ_IN_INITED)
        return CBQ_ERR_ALREADY_INITED;

    if (size <= 0 || size > CBQ_QUEUE_MAX_SIZE)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    if (sizeMode == CBQ_SM_STATIC || sizeMode == CBQ_SM_MAX) {
        iniQueue.sizeMaxLimit = 0;
        iniQueue.sizeMode = sizeMode;
    } else if (sizeMode == CBQ_SM_LIMIT && sizeMaxLimit > 0 && sizeMaxLimit <= CBQ_QUEUE_MAX_SIZE && sizeMaxLimit >= size) {
        iniQueue.sizeMaxLimit = sizeMaxLimit;
        iniQueue.sizeMode = sizeMode;
    } else
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    iniQueue.coArr = (CBQContainer_t*) malloc( sizeof(CBQContainer_t) * size);
    if (iniQueue.coArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* Containers init */
    for (i = 0; i < size; i++)
        if (CBQ_containerInit__(&iniQueue.coArr[i]))
            return CBQ_ERR_MEM_ALLOC_FAILED;

    /* set init status */
    iniQueue.initSt = CBQ_IN_INITED;

    /* send inited struct by pointer */
    *queue = iniQueue;

    CBQ_MSGPRINT("Queue initialized");
    CBQ_DRAWSCHEME_IN(&iniQueue);

    return 0;
}

int CBQ_QueueFree(CBQueue_t* queue)
{
    size_t i;

    BASE_ERR_CHECK(queue);

    /* free args data in containers */
    for (i = 0; i < queue->size; i++)
        CBQ_COARGFREE_P__(queue->coArr[i]);

    /* free containers data */
    free(queue->coArr);

    queue->initSt = CBQ_IN_FREE;

    CBQ_MSGPRINT("Queue freed");

    return 0;
}

int CBQ_ChangeSize(CBQueue_t* queue, int changeTowards, size_t customNewSize)
{
    size_t errSt;

    BASE_ERR_CHECK(queue);

    CBQ_MSGPRINT("Queue size changing...");

    if (changeTowards == CBQ_INC_SIZE) {
        errSt = CBQ_incSize__(queue, 0);
        if (errSt)
            return errSt;

    } else if (changeTowards == CBQ_DEC_SIZE) {
        errSt = CBQ_decSize__(queue, 0);
        if (errSt)
            return errSt;

    /* CBQ_CUSTOM_SIZE */
    } else {
        if (customNewSize <= 0 || customNewSize > CBQ_QUEUE_MAX_SIZE)
            return CBQ_ERR_ARG_OUT_OF_RANGE;

        if (customNewSize == queue->size)
            return 0;

        if (customNewSize > queue->size) {
            errSt = CBQ_incSize__(queue, customNewSize);
            if (errSt)
                return errSt;

        } else {
            errSt = CBQ_decSize__(queue, customNewSize);
            if (errSt)
                return errSt;
        }
    }

    return 0;
}

/* This part is unused. Methods of save and rest queue was complicated
 * for stored pointers of structs, strings and functions.
 * It only makes sense to independently save this data,
 * not using the module for that. Its is not yet able to parse data
 * through a pointers. In the future, tags for variables may appear.
 * However, this can affect the complexity of adding a call to the queue.
 * And there will be a need to register all used types in the future in calls.
 */

/*
int CBQ_SaveState(CBQueue_t* queue, unsigned char* data, size_t* receivedSize)
{
    size_t i, j;
    size_t engSize;
    CBQContainer_t* cbqc;

    BASE_ERR_CHECK(queue);

    CBQ_MSGPRINT("Saving queue state...");

    engSize = CBQ_GetCallAmount(queue);
*/
    /* Getting data size */
    /* (*receivedSize) = GET_BASE_QUEUE_HEADER() + engSize * GET_BASE_CONTAINER_SIZE(); */

    /* The algorithm can move the pointer outside the cells if the queue is full.
     * This is convenient when the queue size changes (increment to a greater extent).
     * But in this case, such a feature is not needed. Therefore we equate pointers.
     * After the shift algorithm, the read pointer is always at zero.
     */
     /*
    CBQ_offsetToBeginning__(queue);
    if (queue->status == CBQ_ST_FULL)
        queue->sId = 0;

    for (i = 0; i < engSize; i++)
        *receivedSize += GET_CONTAINER_ARGS_SIZE(queue->coArr[i]);

    return 0;
}

int CBQ_RestoreState(CBQueue_t* queue, unsigned char* data, size_t size)
{
    size_t i, j;

    BASE_ERR_CHECK(queue);

    CBQ_MSGPRINT("Loading queue state...");

    return 0;
}*/

/* ---------------- Container methods ---------------- */
int CBQ_containerInit__(CBQContainer_t* container)
{
    CBQContainer_t tmpContainer = {
    	.func = NULL,
        .argMax = DEF_CO_ARG,
        .argc = 0

        #ifdef CBQD_SCHEME
        , .label = '-'
        #endif // CBQD_SCHEME
    };

    tmpContainer.args = (CBQArg_t*) malloc( sizeof(CBQArg_t) * DEF_CO_ARG);
    if (tmpContainer.args == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    *container = tmpContainer;

    return 0;
}

int CBQ_incSize__(CBQueue_t* trustedQueue, size_t newIncSize)
{
    int errSt;
    size_t oldSize;

    oldSize = trustedQueue->size;
    if (!newIncSize)
        newIncSize = CBQ_calcNewIncSize__(trustedQueue);

    errSt = CBQ_incSizeCheck__(trustedQueue, newIncSize);
    if (errSt)
        return errSt;

    errSt = CBQ_offsetToBeginning__(trustedQueue);
    if (errSt)
        return errSt;

    errSt = CBQ_reallocSizeToAccepted__(trustedQueue, newIncSize);
    if (errSt)
        return errSt;

    do {
        errSt = CBQ_containerInit__(&trustedQueue->coArr[oldSize]);
        if (errSt)
            return errSt;
        oldSize++;
    } while (oldSize != newIncSize);

    trustedQueue->status = CBQ_ST_STABLE;

    CBQ_MSGPRINT("Queue size incremented");
    CBQ_DRAWSCHEME_IN(trustedQueue);

    return 0;
}

size_t CBQ_calcNewIncSize__(CBQueue_t* trustedQueue)
{
    size_t newIncSize;

    newIncSize = trustedQueue->size + trustedQueue->incSize;

    trustedQueue->incSize <<= 1;
    if (trustedQueue->incSize > MAX_INC_SIZE)
        trustedQueue->incSize = MAX_INC_SIZE;

    return newIncSize;
}

int CBQ_incSizeCheck__(CBQueue_t* trustedQueue, size_t newSize)
{
    if (trustedQueue->sizeMode == CBQ_SM_STATIC)
        return CBQ_ERR_STATIC_SIZE_OVERFLOW;

    if (trustedQueue->sizeMode == CBQ_SM_LIMIT && newSize > trustedQueue->sizeMaxLimit)
        return CBQ_ERR_LIMIT_SIZE_OVERFLOW;

    if (trustedQueue->sizeMode == CBQ_SM_MAX && newSize > CBQ_QUEUE_MAX_SIZE)
        return CBQ_ERR_MAX_SIZE_OVERFLOW;

    return 0;
}

int CBQ_decSize__(CBQueue_t* trustedQueue, size_t newDecSize)
{
    int errSt;
    size_t i;

    /* align or get new size */
    newDecSize = CBQ_decSizeAlignment__(trustedQueue, newDecSize);

    errSt = CBQ_offsetToBeginning__(trustedQueue);
    if (errSt)
        return errSt;

    /* free unused container args */
    i = newDecSize;

    while (i < trustedQueue->size) {
        CBQ_COARGFREE_P__(trustedQueue->coArr[i]);
        i++;
    }

    /* mem reallocation */
    errSt = CBQ_reallocSizeToAccepted__(trustedQueue, newDecSize);
    if (errSt)
        return errSt;

    CBQ_MSGPRINT("Queue size decremented");
    CBQ_DRAWSCHEME_IN(trustedQueue);

    return 0;
}

size_t CBQ_decSizeAlignment__(CBQueue_t* trustedQueue, size_t newDecSize)
{
    size_t tmpSize, callsSize;

    callsSize = CBQ_GetCallAmount(trustedQueue);

    if (newDecSize < MIN_SIZE || newDecSize < callsSize) {
        /* Align new Size */
        tmpSize = (size_t)((long double) callsSize * DEC_BUFF_PERC);

        if (!tmpSize || tmpSize > newDecSize)
            newDecSize = callsSize + MIN_SIZE;
        else
            newDecSize = tmpSize;
    }

    /* it is worth reducing the inc size */
    if (trustedQueue->incSize > INIT_INC_SIZE)
        trustedQueue->incSize >>= 1;

    return newDecSize;
}

int CBQ_reallocSizeToAccepted__(CBQueue_t* trustedQueue, size_t newSize)
{

    trustedQueue->coArr = (CBQContainer_t*) realloc(trustedQueue->coArr, sizeof(CBQContainer_t) * newSize);
    if (trustedQueue->coArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    trustedQueue->size = newSize;

    return 0;
}

/* Offset function was designed to resize to a greater or lesser degree in realloc function.
 * The function puts the queue in order from its various states in which resizing would be difficult.
 */
int CBQ_offsetToBeginning__(CBQueue_t* trustedQueue)
{
    CBQContainer_t* tmpCoArr;
    size_t i, j;

    /* #1 Store pointer which indicates on first cell may set items before at last position.
     * So, all engaged cells will only be up to the last position in queue array.
     * In this way queue always have read pointer which is smaller than store pointer.
     * Case with an empty queue will regulated by #2 stage.
     * The store pointer always indicates on first potentially idle cell and on that
     * situation is necessary offsets values of previous cells which are located on
     * end of queue array.
     */
    if (!trustedQueue->sId)
        trustedQueue->sId = trustedQueue->size;

    /* #2 It is enough to change pointers in an empty queue.
     * Even if store pointer has been changed by #1 stage,
     * the status of the queue will indicate the states with 0 (first) store cell.
     * That pointer will be in its original place.
     */
    if (trustedQueue->status == CBQ_ST_EMPTY) {
        trustedQueue->rId = trustedQueue->sId = 0;
        return 0;
    }

    /* #3 In any situations where read pointer is settled on 0 index no need to offsets.
     * Empty queues are do not reach this stage. And in the moment with a store pointer in #1
     * it will miss pointer to first vacant index like after another one parts.
     */
    if (!trustedQueue->rId)
        return 0;

    /* #4 Simple part where reader with cycle offsets. It moves elements to start
     * at first enganged index (at read pointer) to last. Once the store pointer
     * is on the first idle cell. By #1 processing with filled from read pointer
     * to end of array is possible in this stage.
     */
    if (trustedQueue->rId < trustedQueue->sId) {
        for (i = trustedQueue->rId, j = 0; i < trustedQueue->sId; i++, j++)
            trustedQueue->coArr[j] = trustedQueue->coArr[i];

    /* #5 The last queue states is where st. pointer returns to start cells in array
     * and its index has become less than index of read pointer or equal to that.
     * Situation where any pointers indicates first cell will not occure on that stage.
     * Before moves cells at rd. pointer the stage saves data of engaged cells before
     * st. pointer in heap memory. And after that saved data is moved to cells
     * at rd. pointer. This is similar to the usual swap of values between two variables
     * with using of third (buffer) variable.
     * This is not the best practice but this can be replaced by a better alternative.
     */
    } else {
        tmpCoArr = (CBQContainer_t*) malloc(sizeof(CBQContainer_t) * trustedQueue->sId);
        if (tmpCoArr == NULL)
            return CBQ_ERR_MEM_ALLOC_FAILED;

        for (i = 0; i < trustedQueue->sId; i++)
            tmpCoArr[i] = trustedQueue->coArr[i];

        for (i = trustedQueue->rId, j = 0; i < trustedQueue->size; i++, j++)
            trustedQueue->coArr[j] = trustedQueue->coArr[i];

        for (i = 0; i < trustedQueue->sId; i++, j++)
            trustedQueue->coArr[j] = tmpCoArr[i];

        free(tmpCoArr);
    }

    /* After #4 or #5 stages pointers gets new indexes. New value of st. pointer
     * was recieved after offset last cell. And this indicates the first idle cell
     * not engaged if that queue is full.
     */
    trustedQueue->rId = 0;
    trustedQueue->sId = j;

    return 0;
}

/* ---------------- Container Args Methods ---------------- */
int CBQ_coIncMaxArgSize__(CBQContainer_t* container, size_t newSize)
{
    if (newSize > MAX_CO_ARG)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    container->args = (CBQArg_t*) realloc(container->args, sizeof(CBQArg_t) * newSize);
    if (container->args == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    container->argMax = newSize;

    return 0;
}

/* ---------------- Call Methods ---------------- */
int CBQ_Push(CBQueue_t* queue, QCallback func, int varParamc, CBQArg_t* varParams, int stParamc, CBQArg_t stParams, ...)
{
    size_t i;
    int errSt,
        argcAll;
    CBQContainer_t* container;
    CBQArg_t* args;

    /* base error checking */
    OPT_BASE_ERR_CHECK(queue);

    /* static arg range check */
    if (stParamc < 0)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    /* variable param check (optional), if only varParams pointer is null, vParamc not considered */
    #ifndef NO_VPARAM_CHECK
    if (varParams && varParamc <= 0)
        return CBQ_ERR_VPARAM_VARIANCE;
    #endif

    /* status check */
    if (queue->status == CBQ_ST_FULL) {

        CBQ_MSGPRINT("Queue is full to push");

        errSt = CBQ_incSize__(queue, 0);
        if (errSt)
            return errSt;

        CBQ_MSGPRINT("Size incrementation was automatic");
    }

    /* set into container */
    container = &queue->coArr[queue->sId];
    args = &stParams;

    if (varParams)
        argcAll = stParamc + varParamc;
    else
        argcAll = stParamc;

    if (argcAll > container->argMax) {
        errSt = CBQ_coIncMaxArgSize__(container, argcAll);
        if (errSt)
            return errSt;
    }

    if (stParamc)
        for (i = 0; i < stParamc; i++)
            container->args[i] = args[i];

    /* in CB after static params are variable params*/
    if (varParams)
        for (i = 0; i < varParamc; i++)
            container->args[i + stParamc] = varParams[i];

    container->argc = argcAll;
    container->func = func;

    /* debug for scheme */
    #ifdef CBQD_SCHEME
        container->label = queue->curLetter;
        if (++queue->curLetter > 'Z')
            queue->curLetter = 'A';
    #endif // CBQD_SCHEME

    /* store index */
    queue->sId++;
    if (queue->sId == queue->size)
        queue->sId = 0;

    if (queue->sId == queue->rId)
        queue->status = CBQ_ST_FULL;
    else
        queue->status = CBQ_ST_STABLE;

    CBQ_MSGPRINT("Queue is pushed");
    CBQ_DRAWSCHEME_IN(queue);

    return 0;
}

int CBQ_Exec(CBQueue_t* queue, int* funcRetSt)
{
    CBQContainer_t* container;

    OPT_BASE_ERR_CHECK(queue);

    if (queue->status == CBQ_ST_EMPTY)
        return CBQ_ERR_QUEUE_IS_EMPTY;

    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;

    /* change status as busy */
    queue->execSt = CBQ_EST_EXEC;

    /* inset from container and execute callback function */
    container = &queue->coArr[queue->rId];
    if (funcRetSt == NULL)
        container->func(container->argc, container->args);
    else
        *funcRetSt = container->func(container->argc, container->args);

    /* read index */
    queue->rId++;
    if (queue->rId == queue->size)
        queue->rId = 0;

    if (queue->rId == queue->sId)
        queue->status = CBQ_ST_EMPTY;
    else
        queue->status = CBQ_ST_STABLE;

    /* now queue is free for executing */
    queue->execSt = CBQ_EST_NO_EXEC;

    CBQ_MSGPRINT("Queue is popped");

    #ifdef CBQD_SCHEME
    container->label = '-';
    #endif
    CBQ_DRAWSCHEME_IN(queue);

    return 0;
}

int CBQ_Clear(CBQueue_t* queue)
{
    BASE_ERR_CHECK(queue);

    /* To clear a queue, you can simply shift the pointers
     * to a common index and set the status of an empty queue.
     */
    queue->rId = queue->sId = 0;
    queue->status = CBQ_ST_EMPTY;

    return 0;
}

/* ---------------- Info Methods ---------------- */
size_t CBQ_GetCallAmount(CBQueue_t* queue)
{
    OPT_BASE_ERR_CHECK(queue);

    if (queue->rId < queue->sId)
        return queue->sId - queue->rId;
    else if (queue->status == CBQ_ST_FULL || queue->rId > queue->sId)
        return queue->size - queue->rId + queue->sId;

    return 0;
}

int CBQ_GetFullInfo(CBQueue_t* queue, int* getStatus, size_t* getSize, size_t* getEngagedSize,
    int* getSizeMode, size_t* getSizeMaxLimit)
    {
        BASE_ERR_CHECK(queue);

        if (getStatus)
            *getStatus = queue->status;

        if (getSize)
            *getSize = queue->size;

        if (getEngagedSize)
            *getEngagedSize = CBQ_GetCallAmount(queue);

        if (getSizeMode)
            *getSizeMode = queue->sizeMode;

        if (getSizeMaxLimit)
            *getSizeMaxLimit = queue->sizeMaxLimit;

        return 0;
    }

/* ---------------- Other Methods ---------------- */
char* CBQ_strIntoHeap(const char* str)
{
    char* buff;
    size_t size = 0;

    while (str[size])
        size++;

    buff = (char*) malloc(sizeof (char) * size);

    while(size--)
        buff[size] = str[size];

    return buff;
}

#ifdef CBQD_STATUS
    void CBQ_outDebugSysStatus__(void)
    {
        printf("Debug System status:\n"

        #ifdef CBQD_ASRT
            "* Assert-like macros is active\n"
        #endif

        #ifdef CBQD_SCHEME
            "* Drawing queue scheme is active\n"
        #endif

        #ifdef CBQD_OUTPUTLOG
            "* Output base log is active\n"
        #endif

        "\n");
    }

#endif // CBQD_STATUS

#ifdef CBQD_SCHEME

void CBQ_drawScheme__(CBQueue_t* trustedQueue)
{
    CBQContainer_t* cbqc;

    size_t i;

    printf("Queue scheme:\n");
    for (i = 0; i < trustedQueue->size; i++) {

        #ifdef __unix__
            if (i == trustedQueue->rId && i == trustedQueue->sId)
                printf("\033[35m");
            else if (i == trustedQueue->rId)
                printf("\033[31m");
            else if (i == trustedQueue->sId)
                printf("\033[34m");
        #endif

        cbqc = &trustedQueue->coArr[i];

        if (cbqc->label >= 'A' && cbqc->label <= 'Z')
            printf("%c", (char) cbqc->label);
        else
            printf("-");

        #ifdef __unix__
            printf("\033[0m");
        #endif
    }

    printf("\n");

    #ifndef __unix__
        for (i = 0; i < trustedQueue->size; i++) {
            if (i == trustedQueue->rId && i == trustedQueue->sId)
                printf("b");
            else if (i == trustedQueue->rId)
                printf("r");
            else if (i == trustedQueue->sId)
                printf("s");
            else
                printf(".");
        }

        printf("\n\n");
    #endif

    fflush(stdout);
}

int CBQ_drawScheme_chk__(void* queue)
{
    CBQueue_t* cqueue = (CBQueue_t*) queue;

    BASE_ERR_CHECK(cqueue);

    CBQ_drawScheme__(cqueue);

    return 0;
}

#endif // CBQD_SCHEME
