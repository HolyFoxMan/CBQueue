#include "cbqbuildconf.h"
#include "cbqdebug.h"
#include "cbqueue.h"
#include "cbqlocal.h"
#include "cbqcontainer.h"
#include "cbqcapacity.h"

int CBQ_QueueInit(CBQueue_t* queue, size_t capacity, int incCapacityMode, size_t maxCapacityLimit, unsigned int customInitArgsCapacity)
{
    int errSt;
    CBQueue_t iniQueue = {
        #ifndef NO_EXCEPTIONS_OF_BUSY
        .execSt = CBQ_EST_NO_EXEC,
        #endif
        .capacity = capacity,
        .incCapacity = INIT_INC_CAPACITY,
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

    if (capacity < CBQ_QUEUE_MIN_CAPACITY || capacity > CBQ_QUEUE_MAX_CAPACITY)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    if (customInitArgsCapacity) {
        if (customInitArgsCapacity < MIN_CAP_ARGS || customInitArgsCapacity > MAX_CAP_ARGS)
            return CBQ_ERR_ARG_OUT_OF_RANGE;
        iniQueue.initArgCap = customInitArgsCapacity;
    } else
        iniQueue.initArgCap = INIT_CAP_ARGS;

    if (incCapacityMode == CBQ_SM_STATIC || incCapacityMode == CBQ_SM_MAX) {
        iniQueue.maxCapacityLimit = 0;
        iniQueue.incCapacityMode = incCapacityMode;
    } else if (incCapacityMode == CBQ_SM_LIMIT && maxCapacityLimit > 0 && maxCapacityLimit <= CBQ_QUEUE_MAX_CAPACITY && maxCapacityLimit >= capacity) {
        iniQueue.maxCapacityLimit = maxCapacityLimit;
        iniQueue.incCapacityMode = incCapacityMode;
    } else
        return CBQ_ERR_ARG_OUT_OF_RANGE;    // for incCapacityMode and/or maxCapacityLimit params

    iniQueue.coArr = (CBQContainer_t*) CBQ_MALLOC( sizeof(CBQContainer_t) * capacity);
    if (iniQueue.coArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* Containers init */
    errSt = CBQ_containersRangeInit__(iniQueue.coArr, iniQueue.initArgCap, capacity, REST_MEM);
    if (errSt)
        return errSt;

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
    BASE_ERR_CHECK(queue);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    /* free args data in containers */
    CBQ_containersRangeFree__(queue->coArr, queue->capacity);

    /* free containers data */
    CBQ_MEMFREE(queue->coArr);

    queue->initSt = CBQ_IN_FREE;

    CBQ_MSGPRINT("Queue freed");
    return 0;
}

#ifdef CBQ_ALLOW_V2_METHODS

int CBQ_QueueCopy(CBQueue_t* restrict dest, const CBQueue_t* restrict src)
{
    /* base error checking */
    OPT_BASE_ERR_CHECK(src);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (src->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    if (dest == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;

    if (dest->initSt == CBQ_IN_INITED)
        return CBQ_ERR_ALREADY_INITED;

    CBQContainer_t* tmpCoArr = (CBQContainer_t*) CBQ_MALLOC(src->capacity * sizeof(CBQContainer_t));
    if (tmpCoArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    CBQ_containersCopy__(src->coArr, tmpCoArr, src->capacity);

    for (size_t i = 0; i < src->capacity; i++) {
        tmpCoArr[i].args = (CBQArg_t*) CBQ_MALLOC(src->coArr[i].capacity * sizeof(CBQArg_t));

        if (tmpCoArr[i].args == NULL) {
        #ifdef REST_MEM
            CBQ_containersRangeFree__(tmpCoArr, i);
            CBQ_MEMFREE(tmpCoArr);
            return CBQ_ERR_MEM_BUT_RESTORED;
        #else // REST_MEM
            return CBQ_ERR_MEM_ALLOC_FAILED;
        #endif
        }

        CBQ_copyArgs__(src->coArr[i].args, tmpCoArr[i].args, src->coArr[i].capacity);
    }

    *dest = *src;
    dest->coArr = tmpCoArr;

    return 0;
}

/* for dest initialization or assigning with src */
int CBQ_QueueCorrectMove(CBQueue_t* restrict dest, CBQueue_t* restrict src)
{
    if (dest == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;

    BASE_ERR_CHECK(src);

    if (dest == src)
        return CBQ_ERR_SAME_QUEUE;

    if (dest->initSt == CBQ_IN_INITED)
        CBQ_QueueFree(dest);

    *dest = *src;

    src->coArr = NULL;
    src->initSt = CBQ_IN_FREE;

    return 0;
}

int CBQ_QueueConcat(CBQueue_t* restrict dest, const CBQueue_t* restrict src)
{
    int errSt;
    size_t commonSize, srcSize;
    CBQContainer_t* container;

    OPT_BASE_ERR_CHECK(dest);
    BASE_ERR_CHECK(src);

    if (dest == src)
        return CBQ_ERR_SAME_QUEUE;

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (dest->execSt == CBQ_EST_EXEC || src->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY


    commonSize = CBQ_getSizeByIndexes__(dest) + (srcSize = CBQ_getSizeByIndexes__(src));

    if (dest->capacity < commonSize) {
        errSt = CBQ_incCapacity__(dest, commonSize - dest->capacity, 0);
        if (errSt)
            return errSt;
    }

    for (size_t offset = src->rId, i = 0; i < srcSize; i++, offset = (offset + 1) % src->capacity) {
        container = src->coArr + i;
        errSt = CBQ_PushOnlyVP(dest, container->func, container->argc, container->args);
        if (errSt)
            return errSt;
    }

    return 0;
}

int CBQ_QueueTransfer(CBQueue_t* restrict dest, CBQueue_t* restrict src, size_t count, const int cutByDestLimit, const int cutBySrcSize)
{
    int errSt = 0;

    OPT_BASE_ERR_CHECK(dest);
    BASE_ERR_CHECK(src);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (dest->execSt == CBQ_EST_EXEC || src->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    if (dest == src)
        return CBQ_ERR_SAME_QUEUE;

    if (!count)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    size_t commonSize, destSize, srcSize;
    commonSize = (destSize = CBQ_getSizeByIndexes__(dest)) + (srcSize = CBQ_getSizeByIndexes__(src));

    if (!srcSize)
        return CBQ_ERR_QUEUE_IS_EMPTY;

    if (count > srcSize) {
        if (!cutBySrcSize)
            return CBQ_ERR_COUNT_NOT_FIT_IN_SIZE;
        count = srcSize;
    }

    commonSize = destSize + count;
    if (commonSize > dest->capacity) {
        errSt = CBQ_incCapacity__(dest, commonSize - dest->capacity, cutByDestLimit);
        if (errSt)
            return errSt;

        count = dest->capacity;
    }

    for (CBQContainer_t* container; count; count--) {
        container = src->coArr + src->rId;
        errSt = CBQ_PushOnlyVP(dest, container->func, container->argc, container->args);
        if (errSt)
            return errSt;

        src->rId++;
        if (src->rId == src->capacity)
            src->rId = 0;
    }

    if (src->rId == src->sId)
        src->status = CBQ_ST_EMPTY;
    else if (src->status == CBQ_ST_FULL) // still some leftover
        src->status = CBQ_ST_STABLE;

    return 0;
}

int CBQ_Skip(CBQueue_t* queue, size_t count, const int cutBySize, const int reverseOrder)
{
    OPT_BASE_ERR_CHECK(queue);

    size_t size;
    size = CBQ_getSizeByIndexes__(queue);

    if (!count)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    if (count > size) {
        if (!(cutBySize && size)) {
            return CBQ_ERR_COUNT_NOT_FIT_IN_SIZE;
        }
        count = size;
    }

    if (!reverseOrder)
        queue->rId = (queue->rId + count) % queue->capacity;    // at front
    else {                                                      // at back
        if (queue->sId < count) {   // or sId < rId
            count -= queue->sId;
            queue->sId = queue->capacity;
        }
        queue->sId -= count;
    }

    if (queue->rId == queue->sId)
        queue->status = CBQ_ST_EMPTY;
    else if (queue->status == CBQ_ST_FULL) // still some leftover
        queue->status = CBQ_ST_STABLE;

    return 0;
}

#endif // Version 2

int CBQ_ChangeInitArgsCapByCustom(CBQueue_t* queue, unsigned int customInitCapacity)
{
    OPT_BASE_ERR_CHECK(queue);

    if (customInitCapacity < MIN_CAP_ARGS || customInitCapacity > MAX_CAP_ARGS)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    if (customInitCapacity == queue->initArgCap)
        return CBQ_ERR_INITCAP_IS_IDENTICAL;

    queue->initArgCap = customInitCapacity;

    CBQ_MSGPRINT("Queue init args cap is changed");
    return 0;
}

int CBQ_EqualizeArgsCapByCustom(CBQueue_t* queue, unsigned int customCapacity, const int passNonModifiableArgs)
{
    size_t size, MAY_REG offset, MAY_REG i;
    CBQContainer_t* container;
    int errSt;

    OPT_BASE_ERR_CHECK(queue);

    #ifndef NO_EXCEPTIONS_OF_BUSY
        if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    if (customCapacity < MIN_CAP_ARGS || customCapacity > MAX_CAP_ARGS)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    CBQ_MSGPRINT("Queue call args cap is equalize...");

    size = CBQ_getSizeByIndexes__(queue);

    for (i = 0, offset = queue->rId; i < size; i++, offset = (offset + 1) % queue->capacity) { // % helps to loop ptr into capacity frames

        container = queue->coArr + offset;

        if (customCapacity < container->argc) {
            if (passNonModifiableArgs)
                continue;
            else
                return CBQ_ERR_HURTS_USED_ARGS;
        } else if (customCapacity == container->argc)
            continue;
        else {
            errSt = CBQ_changeArgsCapacity__(container, customCapacity, 1);
            if (errSt)
                return errSt;
        }
    }

    for (i = 0, offset = queue->sId; i < queue->capacity - size; i++, offset = (offset + 1) % queue->capacity) {

        container = queue->coArr + offset;

        if (customCapacity == container->argc)
            continue;
        else {
            errSt = CBQ_changeArgsCapacity__(container, customCapacity, 0);
            if (errSt)
                return errSt;
        }
    }

    CBQ_MSGPRINT("Queue call args have equalized capacity");
    return 0;
}


/* ---------------- Call Methods ---------------- */
__cdecl int CBQ_Push(CBQueue_t* queue, QCallback func, unsigned int varParamc, CBQArg_t* varParams, unsigned int stParamc, CBQArg_t stParams, ...)
{
    int errSt;
    unsigned int argcAll;
    CBQContainer_t* container;

    /* base error checking */
    OPT_BASE_ERR_CHECK(queue);

    /* variable param check (optional), if only varParams pointer is null, vParamc not considered */
    #ifndef NO_VPARAM_CHECK
    if (varParams && !varParamc)
        return CBQ_ERR_VPARAM_VARIANCE;
    #endif

    /* status check */
    if (queue->status == CBQ_ST_FULL) {

        CBQ_MSGPRINT("Queue is full to push");

        errSt = CBQ_incCapacity__(queue, 0, 1);
        if (errSt)
            return errSt;

        CBQ_MSGPRINT("Capacity incrementation was automatic");
    }

    /* set into container */
    container = &queue->coArr[queue->sId];

    if (varParams)
        argcAll = stParamc + varParamc;
    else
        argcAll = stParamc;

    if (argcAll > container->capacity) {

        CBQ_MSGPRINT("Auto inc arg capacity...");

        errSt = CBQ_changeArgsCapacity__(container, argcAll, 0);
        if (errSt)
            return errSt;
    }

    if (stParamc)
        CBQ_copyArgs__(&stParams, container->args, stParamc);

    /* in CB after static params are variable params*/
    if (varParams)
        CBQ_copyArgs__(varParams, container->args + stParamc, varParamc);

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
    if (queue->sId == queue->capacity)
        queue->sId = 0;

    if (queue->sId == queue->rId)
        queue->status = CBQ_ST_FULL;
    else
        queue->status = CBQ_ST_STABLE;

    CBQ_MSGPRINT("Queue is pushed");
    CBQ_DRAWSCHEME_IN(queue);

    return 0;
}

int CBQ_PushOnlyVP(CBQueue_t* queue, QCallback func, unsigned int varParamc, CBQArg_t* varParams)
{
    int errSt;
    CBQContainer_t* container;

    /* base error checking */
    OPT_BASE_ERR_CHECK(queue);

    /* variable param check (optional), if only varParams pointer is null, vParamc not considered */
    #ifndef NO_VPARAM_CHECK
    if (varParams == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;
    if (!varParamc)
        return CBQ_ERR_VPARAM_VARIANCE;
    #endif

    /* status check */
    if (queue->status == CBQ_ST_FULL) {

        CBQ_MSGPRINT("Queue is full to push");

        errSt = CBQ_incCapacity__(queue, 0, 1);
        if (errSt)
            return errSt;

        CBQ_MSGPRINT("Capacity incrementation was automatic");
    }

    /* set into container */
    container = queue->coArr + queue->sId;  // container = &queue->coArr[queue->sId]

    if (varParamc > container->capacity) {

            CBQ_MSGPRINT("Auto inc arg capacity...");

            errSt = CBQ_changeArgsCapacity__(container, varParamc, 0);
            if (errSt)
                return errSt;
    }

    CBQ_copyArgs__(varParams, container->args, varParamc);

    container->argc = varParamc;
    container->func = func;

    /* debug for scheme */
    #ifdef CBQD_SCHEME
        container->label = queue->curLetter;
        if (++queue->curLetter > 'Z')
            queue->curLetter = 'A';
    #endif // CBQD_SCHEME

    /* store index */
    queue->sId++;
    if (queue->sId == queue->capacity)
        queue->sId = 0;

    if (queue->sId == queue->rId)
        queue->status = CBQ_ST_FULL;
    else
        queue->status = CBQ_ST_STABLE;

    CBQ_MSGPRINT("Queue is pushed");
    CBQ_DRAWSCHEME_IN(queue);

    return 0;
}

int CBQ_PushVoid(CBQueue_t* queue, QCallback func)
{
    /* base error checking */
    OPT_BASE_ERR_CHECK(queue);

    /* status check */
    if (queue->status == CBQ_ST_FULL) {

        int errSt;
        CBQ_MSGPRINT("Queue is full to push");

        errSt = CBQ_incCapacity__(queue, 0, 1);
        if (errSt)
            return errSt;

        CBQ_MSGPRINT("Capacity incrementation was automatic");
    }

    /* set into container only func */
    (queue->coArr + queue->sId)->func = func;
    (queue->coArr + queue->sId)->argc = 0;

    /* debug for scheme */
    #ifdef CBQD_SCHEME
        (queue->coArr + queue->sId)->label = queue->curLetter;
        if (++queue->curLetter > 'Z')
            queue->curLetter = 'A';
    #endif // CBQD_SCHEME

    /* store index */
    queue->sId++;
    if (queue->sId == queue->capacity)
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

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;

    /* change status as busy */
    queue->execSt = CBQ_EST_EXEC;
    #endif // NO_EXCEPTIONS_OF_BUSY

    /* inset from container and execute callback function */
    container = queue->coArr + queue->rId;
    if (funcRetSt == NULL)
        container->func( (int) container->argc, container->args);
    else
        *funcRetSt = container->func( (int) container->argc, container->args);

    #ifdef CBQD_SCHEME
    queue->coArr[queue->rId].label = '-';
    #endif

    /* read index */
    queue->rId++;
    if (queue->rId == queue->capacity)
        queue->rId = 0;

    if (queue->rId == queue->sId)
        queue->status = CBQ_ST_EMPTY;
    else
        queue->status = CBQ_ST_STABLE;

    #ifndef NO_EXCEPTIONS_OF_BUSY
    /* now queue is free for executing */
    queue->execSt = CBQ_EST_NO_EXEC;
    #endif // NO_EXCEPTIONS_OF_BUSY

    CBQ_MSGPRINT("Queue is popped");

    CBQ_DRAWSCHEME_IN(queue);

    return 0;
}

int CBQ_Clear(CBQueue_t* queue)
{
    OPT_BASE_ERR_CHECK(queue);

    /* To clear a queue, you can simply shift the pointers
     * to a common index and set the status of an empty queue.
     */
    queue->rId = queue->sId = 0;
    queue->status = CBQ_ST_EMPTY;

    #ifdef CBQD_SCHEME
    for (size_t i = 0; i < queue->capacity; i++)
        queue->coArr[i].label = '-';
    #endif // CBQD_SCHEME
    return 0;
}

/* ---------------- Info Methods ---------------- */
int CBQ_GetSize(const CBQueue_t* queue, size_t* size)
{
    OPT_BASE_ERR_CHECK(queue);
    if (size == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;
    *size = CBQ_getSizeByIndexes__(queue);
    return 0;
}

int CBQ_GetCapacityInBytes(const CBQueue_t* queue, size_t* byteCapacity)
{
    size_t bCapacity;

    OPT_BASE_ERR_CHECK(queue);
    if (byteCapacity == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;

    bCapacity = sizeof(CBQueue_t) + queue->capacity * sizeof(CBQContainer_t);
    for (size_t i = 0; i < queue->capacity; i++)
        bCapacity += (size_t) queue->coArr[i].capacity * sizeof(CBQArg_t);

    *byteCapacity = bCapacity;
    return 0;
}

int CBQ_GetDetailedInfo(const CBQueue_t* queue, size_t *restrict getCapacity, size_t *restrict getSize,
    int *restrict getIncCapacityMode, size_t *restrict getMaxCapacityLimit, size_t *restrict getCapacityInBytes)
    {
        OPT_BASE_ERR_CHECK(queue);

        if (getCapacity)
            *getCapacity = queue->capacity;

        if (getSize)
            *getSize = CBQ_getSizeByIndexes__(queue);

        if (getIncCapacityMode)
            *getIncCapacityMode = queue->incCapacityMode;

        if (getMaxCapacityLimit)
            *getMaxCapacityLimit = queue->maxCapacityLimit;

        if (getCapacityInBytes)
            CBQ_GetCapacityInBytes(queue, getCapacityInBytes);

        return 0;
    }

/* ---------------- Other Methods ---------------- */
char* CBQ_strIntoHeap(const char* str)
{
    char* buff;
    size_t len = 0;

    while (str[len])
        len++;

    buff = (char*) CBQ_MALLOC(sizeof (char) * len);

    while (len--)
        buff[len] = str[len];

    return buff;
}
