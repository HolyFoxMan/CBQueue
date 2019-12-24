#include "cbqlocal.h"

/* ---------------- local methods declaration ---------------- */
/* Container Methods */
static int CBQ_containerInit__(CBQContainer_t*);
static int CBQ_reallocSize__(CBQueue_t*, size_t);
static int CBQ_orderingDividedSegs__(CBQueue_t*, size_t*);
static int CBQ_orderingDividedSegsInFullQueue__(CBQueue_t*);
static void CBQ_containersSwapping__(MAY_REG CBQContainer_t*, MAY_REG CBQContainer_t*, MAY_REG size_t, int);
static void CBQ_containersCopy__(MAY_REG const CBQContainer_t *restrict, MAY_REG CBQContainer_t *restrict, MAY_REG size_t);
static int CBQ_containersRangeInit__(CBQContainer_t*, size_t, int);
static void CBQ_containersRangeFree__(MAY_REG CBQContainer_t*, MAY_REG size_t);
static void CBQ_IncIterSizeChange__(CBQueue_t*, int);

/* Incrementation */
static int CBQ_incSizeCheck__(CBQueue_t*, size_t);
static int CBQ_incSize__(CBQueue_t*, size_t);

/* Decrementation */
static int CBQ_decSize__(CBQueue_t*, size_t, int);

/* Arguments */
static int CBQ_coIncArgsCapacity__(CBQContainer_t*, unsigned int);
static void CBQ_coCopyArgs__(MAY_REG const CBQArg_t *restrict, MAY_REG CBQArg_t *restrict, MAY_REG unsigned int);

/* Callbacks */
static int CBQ_setTimeoutFrame__(int, CBQArg_t*);

//  DEBUG MACRO-FUNCS                                                         //
////////////////////////////////////////////////////////////////////////////////
/* inline methods by macros */
#ifdef CBQD_SCHEME

    /* void* type because thats declared in debug header,
     * which was included before CBQueue_t* type definition
     */
    int CBQ_drawScheme_chk__(void*);
    static void CBQ_drawScheme__(CBQueue_t*);
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
//                                                                            //

/* ---------------- Base methods ---------------- */
int CBQ_QueueInit(CBQueue_t* queue, size_t size, int incSizeMode, size_t maxSizeLimit)
{
    int errSt;
    CBQueue_t iniQueue = {
        #ifndef NO_EXCEPTIONS_OF_BUSY
        .execSt = CBQ_EST_NO_EXEC,
        #endif
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

    if (incSizeMode == CBQ_SM_STATIC || incSizeMode == CBQ_SM_MAX) {
        iniQueue.maxSizeLimit = 0;
        iniQueue.incSizeMode = incSizeMode;
    } else if (incSizeMode == CBQ_SM_LIMIT && maxSizeLimit > 0 && maxSizeLimit <= CBQ_QUEUE_MAX_SIZE && maxSizeLimit >= size) {
        iniQueue.maxSizeLimit = maxSizeLimit;
        iniQueue.incSizeMode = incSizeMode;
    } else
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    iniQueue.coArr = (CBQContainer_t*) CBQ_MALLOC( sizeof(CBQContainer_t) * size);
    if (iniQueue.coArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* Containers init */
    errSt = CBQ_containersRangeInit__(iniQueue.coArr, size, REST_MEM);
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
    CBQ_containersRangeFree__(queue->coArr, queue->size);

    /* free containers data */
    CBQ_MEMFREE(queue->coArr);

    queue->initSt = CBQ_IN_FREE;

    CBQ_MSGPRINT("Queue freed");

    return 0;
}

int CBQ_ChangeSize(CBQueue_t* queue, int changeTowards, size_t customNewSize)
{
    size_t errSt;

    BASE_ERR_CHECK(queue);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    CBQ_MSGPRINT("Queue size changing...");

    /* by selected mode with auto-dec/inc params */
    if (changeTowards == CBQ_INC_SIZE) {
        errSt = CBQ_incSize__(queue, 0);
        if (errSt)
            return errSt;

    } else if (changeTowards == CBQ_DEC_SIZE) {
        errSt = CBQ_decSize__(queue, 0, 1);
        if (errSt)
            return errSt;

    /* by new custom size */
    } else {
        if (customNewSize <= 0 || customNewSize > CBQ_QUEUE_MAX_SIZE)
            return CBQ_ERR_ARG_OUT_OF_RANGE;

        ssize_t delta = (ssize_t) (customNewSize - queue->size);

        if (!delta)
            return CBQ_ERR_CUR_CH_SIZE_NOT_AFFECT;
        /* inc */
        else if (delta > 0) {
            errSt = CBQ_incSize__(queue, delta);
            if (errSt)
                return errSt;
        /* dec */
        } else {
            errSt = CBQ_decSize__(queue, -delta, 1);
            if (errSt)
                return errSt;
        }
    }

    return 0;
}


int CBQ_ChangeIncSizeMode(CBQueue_t* queue, int newIncSizeMode, size_t newMaxSizeLimit, int tryToAdaptSize, int adaptMaxSizeLimit)
{
    int errSt;

    BASE_ERR_CHECK(queue);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    CBQ_MSGPRINT("Queue size mode changing...");

    if (newIncSizeMode != CBQ_SM_LIMIT) {
        queue->maxSizeLimit = 0;
        queue->incSizeMode = newIncSizeMode;
        return 0;
    } else {
        if (newMaxSizeLimit < CBQ_QUEUE_MIN_SIZE || newMaxSizeLimit > CBQ_QUEUE_MAX_SIZE)
            return CBQ_ERR_ARG_OUT_OF_RANGE;

        /* size does not fit into new limits */
        if (newMaxSizeLimit < queue->size) {

            if (tryToAdaptSize) {
                errSt = CBQ_decSize__(queue, queue->size - newMaxSizeLimit, adaptMaxSizeLimit);
                /* without size max limit adaptation flag the function may just give an error and close */
                if (errSt)
                    return errSt;

                /* if it was not possible to equalize even after size decrementing */
                if (newMaxSizeLimit != queue->size)
                    newMaxSizeLimit = queue->size;
            }
            else if (adaptMaxSizeLimit)
                newMaxSizeLimit = queue->size;
            else
                return CBQ_ERR_SIZE_NOT_FIT_IN_LIMIT;
        }

        queue->incSizeMode = CBQ_SM_LIMIT;
        queue->maxSizeLimit = newMaxSizeLimit;
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
        .capacity = INIT_CAP_ARGS,
        .argc = 0

        #ifdef CBQD_SCHEME
        , .label = '-'
        #endif // CBQD_SCHEME
    };

    tmpContainer.args = (CBQArg_t*) CBQ_MALLOC( sizeof(CBQArg_t) * INIT_CAP_ARGS);
    if (tmpContainer.args == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    *container = tmpContainer;

    return 0;
}

int CBQ_containersRangeInit__(CBQContainer_t* coFirst, size_t len, int restore_pos_fail)
{
    MAY_REG CBQContainer_t* container = coFirst;
    MAY_REG size_t remLen = len;
    int errSt = 0;

    do {
        if (CBQ_containerInit__(container)) {
            errSt = CBQ_ERR_MEM_ALLOC_FAILED;
            break;
        }
        container++;
    } while (--remLen);

    if (errSt) {
        if (!restore_pos_fail)
            return CBQ_ERR_MEM_ALLOC_FAILED;

        CBQ_containersRangeFree__(coFirst, len - remLen);

        return CBQ_ERR_MEM_BUT_RESTORED;
    }

    return 0;
}

void CBQ_containersRangeFree__(MAY_REG CBQContainer_t* container, MAY_REG size_t len)
{
    do {
        CBQ_MEMFREE(container->args);
        container++;
    } while (--len);
}

int CBQ_incSize__(CBQueue_t* trustedQueue, size_t delta)
{
    int errSt;
    int usedGeneratedIncrement;

    /* Checking new size */
    if (!delta) {
        usedGeneratedIncrement = 1;
        delta = trustedQueue->incSize;
    }
    else if (delta > CBQ_QUEUE_MAX_SIZE)
        return CBQ_ERR_ARG_OUT_OF_RANGE;
    else
        usedGeneratedIncrement = 0;

    errSt = CBQ_incSizeCheck__(trustedQueue, trustedQueue->size + delta);
    if (errSt)
        return errSt;

    /* Ordering cells */
    if (!trustedQueue->sId) {
        if (trustedQueue->status != CBQ_ST_EMPTY)
            trustedQueue->sId = trustedQueue->size;
    }
    else if (trustedQueue->sId < trustedQueue->rId) {    // when segments of occupied cells are divided
        size_t new_sId;
        errSt = CBQ_orderingDividedSegs__(trustedQueue, &new_sId);
        if (errSt)
            return errSt;
        trustedQueue->rId = 0;
        trustedQueue->sId = new_sId;
    }
    else if (trustedQueue->status == CBQ_ST_FULL)  { // segments are also divided and there are no empty cells.
        errSt = CBQ_orderingDividedSegsInFullQueue__(trustedQueue);
        if (errSt)
            return errSt;
        trustedQueue->rId = 0;
        trustedQueue->sId = trustedQueue->size;
    }

    /* Realloc mem to new size */
    errSt = CBQ_reallocSize__(trustedQueue, trustedQueue->size + delta);
    if (errSt)
        return errSt;

    /* Init new allocated containers */
    errSt = CBQ_containersRangeInit__(trustedQueue->coArr + trustedQueue->size, delta, REST_MEM);
    if (errSt) {

        if (errSt != CBQ_ERR_MEM_BUT_RESTORED)
            return errSt;

        if (CBQ_reallocSize__(trustedQueue, trustedQueue->size))
            return CBQ_ERR_MEM_ALLOC_FAILED; // totally failed

        if (trustedQueue->sId == trustedQueue->size)
            trustedQueue->sId = 0; // if in the past the queue was full

        return errSt;
    }

    if (usedGeneratedIncrement)
        CBQ_IncIterSizeChange__(trustedQueue, 1); // Up

    /* Sets new incremented size and status */
    trustedQueue->size += delta;

    if (trustedQueue->status == CBQ_ST_FULL)
        trustedQueue->status = CBQ_ST_STABLE;

    CBQ_MSGPRINT("Queue size incremented");
    CBQ_DRAWSCHEME_IN(trustedQueue);

    return 0;
}

void CBQ_IncIterSizeChange__(CBQueue_t* trustedQueue, int direction)
{
    if (direction)  { // Up
        trustedQueue->incSize <<= 1;
        if (trustedQueue->incSize > MAX_INC_SIZE)
            trustedQueue->incSize = MAX_INC_SIZE;
    } else {    // Down
        trustedQueue->incSize >>= 1;
        if (trustedQueue->incSize < INIT_INC_SIZE)
            trustedQueue->incSize = INIT_INC_SIZE;
    }
}

int CBQ_incSizeCheck__(CBQueue_t* trustedQueue, size_t delta)
{
    if (trustedQueue->incSizeMode == CBQ_SM_STATIC)
        return CBQ_ERR_STATIC_SIZE_OVERFLOW;

    if (trustedQueue->incSizeMode == CBQ_SM_LIMIT && (trustedQueue->size + delta) > trustedQueue->maxSizeLimit)
        return CBQ_ERR_LIMIT_SIZE_OVERFLOW;

    if (trustedQueue->incSizeMode == CBQ_SM_MAX && (trustedQueue->size + delta) > CBQ_QUEUE_MAX_SIZE)
        return CBQ_ERR_MAX_SIZE_OVERFLOW;

    return 0;
}

int CBQ_decSize__(CBQueue_t* trustedQueue, size_t delta, int alignToUsedCells)
{
    int errSt;
    size_t engCellSize;
    ssize_t remainder;

    /* Check and align delta */
    engCellSize = CBQ_GetCallAmount(trustedQueue);

    if (!delta)
        delta = trustedQueue->size - engCellSize;
    else if (delta > CBQ_QUEUE_MAX_SIZE)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    remainder = (ssize_t) trustedQueue->size - (ssize_t) (engCellSize > CBQ_QUEUE_MIN_SIZE? engCellSize : CBQ_QUEUE_MIN_SIZE);
    if (!remainder)
        return CBQ_ERR_CUR_CH_SIZE_NOT_AFFECT;

    remainder -= delta;
    if (remainder < 0) {
        if (alignToUsedCells)
            delta += remainder; // to delta balance
        else
            return CBQ_ERR_ENGCELLS_NOT_FIT_IN_NEWSIZE;
    }

    /* Offset or ordeing cells (for 4 cases)*/
    /* ---b--- -> b------ */
    if (trustedQueue->status == CBQ_ST_EMPTY)
        trustedQueue->rId = trustedQueue->sId = 0;
    else if (trustedQueue->rId) {

    /* s--r+++ -> ---r+++s */
        if (!trustedQueue->sId)
            trustedQueue->sId = trustedQueue->size;

    /* --r++s- -> r++s---   (if for example delta == 3, size - sId == 2) */
        if (trustedQueue->rId < trustedQueue->sId && (trustedQueue->size - trustedQueue->sId < delta))
            CBQ_containersSwapping__(trustedQueue->coArr + (trustedQueue->rId), trustedQueue->coArr, engCellSize, 0);
    /* ++s--r+ -> r+++s-- */
        else {
            errSt = CBQ_orderingDividedSegs__(trustedQueue, NULL);
            if (errSt)
                return errSt;
        }
        trustedQueue->rId = 0;
    }

    /* Free unused container args */
    CBQ_containersRangeFree__(trustedQueue->coArr + (trustedQueue->size - delta), delta);

    /* Mem reallocation */
    errSt = CBQ_reallocSize__(trustedQueue, trustedQueue->size - delta);
    if (errSt) {    // mem alloc error

        #if REST_MEM == 1
        int errStRest = 0;
        errStRest = CBQ_containersRangeInit__(trustedQueue->coArr, delta, 1);
        if (!errStRest)
            return CBQ_ERR_MEM_BUT_RESTORED;
        #endif // REST_MEM

        return errSt;
    }

    CBQ_IncIterSizeChange__(trustedQueue, 0); // when reducing the size, it is logical to reduce the incSize var

    /* Sets new size and check sId */
    trustedQueue->size -= delta;
    if (!remainder) { // no free cells left
        trustedQueue->sId = 0;
        trustedQueue->status = CBQ_ST_FULL;
    }

    CBQ_MSGPRINT("Queue size decremented");
    CBQ_DRAWSCHEME_IN(trustedQueue);

    return 0;
}

int CBQ_reallocSize__(CBQueue_t* trustedQueue, size_t newSize)
{
    void* reallocp; // for safety old data

    reallocp = CBQ_REALLOC(trustedQueue->coArr, sizeof(CBQContainer_t) * newSize);
    if (reallocp == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    trustedQueue->coArr = (CBQContainer_t*) reallocp;

    return 0;
}

/* ++s------r+++++++
 * ++-------~~~~~~~~    (move cells in temp at r point)
 * ~~-------~~~~~~~~    (in temp all engaged cells)
 *           -------    (swap free cells with last by reverse iterations)
 * ++++++++++-------    (move from temp)
 * r+++++++++s------
 * The last queue states is where st. pointer returns to start cells in array
 * and its index has become less than index of read pointer or equal to that.
 * Situation where any pointers indicates first cell will not occure on that stage.
 * There are two options - when we have free cells or not.
 * In the first case all busy cells are written in restored order into allocated memory.
 * Then the free cells are shifted to the end of the array by swapping.
 * Here the situation is similar to offset, but now the cells are changing from the end.
 * After all, we copy back the occupied cells from the beginning.
 */
int CBQ_orderingDividedSegs__(CBQueue_t* trustedQueue, size_t* engCellSizeP)
{
    CBQContainer_t* tmpCoArr;
    size_t engCellsSegSize;

    engCellsSegSize = CBQ_GetCallAmount(trustedQueue); // as new sId pos

    tmpCoArr = (CBQContainer_t*) CBQ_MALLOC(sizeof(CBQContainer_t) * engCellsSegSize);
    if (tmpCoArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* copy cells between rId and last cell */
    CBQ_containersCopy__(trustedQueue->coArr + (trustedQueue->rId), tmpCoArr, trustedQueue->size - trustedQueue->rId);
    /* copy cells before sId into next free cells */
    CBQ_containersCopy__(trustedQueue->coArr, tmpCoArr + (trustedQueue->size - trustedQueue->rId), trustedQueue->sId);
    /* copy space of free cells to end of array with swap in reverse (to avoid data overlay) */
    CBQ_containersSwapping__(trustedQueue->coArr + (trustedQueue->rId - 1), // at last free cell
                             trustedQueue->coArr + (trustedQueue->size - 1), trustedQueue->size - engCellsSegSize, 1);
    /* copy to start array from temp */
    CBQ_containersCopy__(tmpCoArr, trustedQueue->coArr, engCellsSegSize);

    CBQ_MEMFREE(tmpCoArr);

    if (engCellSizeP)
        *engCellSizeP = engCellsSegSize;
    return 0;
}

/* +++++++++b+++++++
 * +++++++++~~~~~~~~
 * ~~~~~~~~~~~~~~~~~    (no free cells for swapping)
 * +++++++++++++++++
 * r++++++++++++++++s
 * Looks like ordering with divided segments, but this requires less memory and cycles.
 * We reserve the first half of the cells, shift the second to the beginning
 * and copy the first at the end.
 */
int CBQ_orderingDividedSegsInFullQueue__(CBQueue_t* trustedQueue)
{
    CBQContainer_t* tmpCoArr;
    size_t sizeOfSegment;

    tmpCoArr = (CBQContainer_t*) CBQ_MALLOC(sizeof(CBQContainer_t) * trustedQueue->sId);
    if (tmpCoArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* copy cells before sId into temp */
    CBQ_containersCopy__(trustedQueue->coArr, tmpCoArr, trustedQueue->sId);
    sizeOfSegment = trustedQueue->size - trustedQueue->rId; // get size of second part
    /* swap cells with second part */
    CBQ_containersSwapping__(trustedQueue->coArr + (trustedQueue->rId), trustedQueue->coArr, sizeOfSegment, 0);
    /* copy first part into place after seconf part */
    CBQ_containersCopy__(tmpCoArr, trustedQueue->coArr + (sizeOfSegment), trustedQueue->sId);

    CBQ_MEMFREE(tmpCoArr);

    return 0;
}

/* Accelerated cycle
 * srcp, destp - pointers of areas in queue
 * srcp - point of source cells
 * destp - where are they moving
 * tmpCo - for swaping memory info (not pointer)
 */
/* POT_REG - potential register var, same as register, if 64 compile */
void CBQ_containersSwapping__(MAY_REG CBQContainer_t* srcp, MAY_REG CBQContainer_t* destp, MAY_REG size_t len, int reverse_iter)
{
    CBQContainer_t tmpCo;

    if (reverse_iter)
        do {    // data swap (memory information)
            SWAP_BY_TEMP(*srcp, *destp, tmpCo);
            --srcp, --destp;
        } while (--len);
    else
        do {    // data swap (memory information)
            SWAP_BY_TEMP(*srcp, *destp, tmpCo);
            ++srcp, ++destp;
        } while (--len);
}

void CBQ_containersCopy__(MAY_REG const CBQContainer_t *restrict srcp, MAY_REG CBQContainer_t *restrict destp, MAY_REG size_t len)
{
    do {
        *destp++ = *srcp++;
    } while (--len);
}

/* ---------------- Args Methods ---------------- */
int CBQ_coIncArgsCapacity__(CBQContainer_t* container, unsigned int newCapacity)
{
    void* reallocp;

    if (newCapacity > MAX_CAP_ARGS)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    reallocp = CBQ_REALLOC(container->args, sizeof(CBQArg_t) * (size_t) newCapacity);
    if (reallocp == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    container->args = (CBQArg_t*) reallocp;
    container->capacity = newCapacity;

    return 0;
}

void CBQ_coCopyArgs__(MAY_REG const CBQArg_t *restrict src, MAY_REG CBQArg_t *restrict dest, MAY_REG unsigned int num)
{
    do {
        *dest++ = *src++;
    } while (--num);
}

/* ---------------- Call Methods ---------------- */
int CBQ_Push(CBQueue_t* queue, QCallback func, unsigned int varParamc, CBQArg_t* varParams, unsigned int stParamc, CBQArg_t stParams, ...)
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

        errSt = CBQ_incSize__(queue, 0);
        if (errSt)
            return errSt;

        CBQ_MSGPRINT("Size incrementation was automatic");
    }

    /* set into container */
    container = &queue->coArr[queue->sId];

    if (varParams)
        argcAll = stParamc + varParamc;
    else
        argcAll = stParamc;

    if (argcAll > container->capacity) {
        errSt = CBQ_coIncArgsCapacity__(container, argcAll);
        if (errSt)
            return errSt;
    }

    if (stParamc)
        CBQ_coCopyArgs__(&stParams, container->args, stParamc);

    /* in CB after static params are variable params*/
    if (varParams)
        CBQ_coCopyArgs__(varParams, container->args + stParamc, varParamc);

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

int CBQ_PushOnlyVP(CBQueue_t* queue, QCallback func, unsigned int varParamc, CBQArg_t* varParams)
{
    int errSt;
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

        errSt = CBQ_incSize__(queue, 0);
        if (errSt)
            return errSt;

        CBQ_MSGPRINT("Size incrementation was automatic");
    }

    /* set into container */
    container = queue->coArr + queue->sId;  // container = &queue->coArr[queue->sId]

    if (varParams) {

        if (varParamc > container->capacity) {
            errSt = CBQ_coIncArgsCapacity__(container, varParamc);
            if (errSt)
                return errSt;
        }

        CBQ_coCopyArgs__(varParams, container->args, varParamc);
    }

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

inline int CBQ_Exec(CBQueue_t* queue, int* funcRetSt)
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
    container = &queue->coArr[queue->rId];
    if (funcRetSt == NULL)
        container->func(container->argc, container->args);
    else
        *funcRetSt = container->func( (int) container->argc, container->args);

    #ifdef CBQD_SCHEME
    queue->coArr[queue->rId].label = '-';
    #endif

    /* read index */
    queue->rId++;
    if (queue->rId == queue->size)
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

size_t CBQ_GetSizeInBytes(CBQueue_t* queue)
{
    size_t bSize;

    OPT_BASE_ERR_CHECK(queue);

    bSize = sizeof(CBQueue_t) + queue->size * sizeof(CBQContainer_t);
    for (size_t i = 0; i < queue->size; i++)
        bSize += (size_t) queue->coArr[i].capacity * sizeof(CBQArg_t);

    return bSize;
}

int CBQ_GetFullInfo(CBQueue_t* queue, int* getStatus, size_t* getSize, size_t* getEngagedSize,
    int* getIncSizeMode, size_t* getMaxSizeLimit, size_t* getSizeInBytes)
    {
        BASE_ERR_CHECK(queue);

        if (getStatus)
            *getStatus = queue->status;

        if (getSize)
            *getSize = queue->size;

        if (getEngagedSize)
            *getEngagedSize = CBQ_GetCallAmount(queue);

        if (getIncSizeMode)
            *getIncSizeMode = queue->incSizeMode;

        if (getMaxSizeLimit)
            *getMaxSizeLimit = queue->maxSizeLimit;

        if (getSizeInBytes)
            *getSizeInBytes = CBQ_GetSizeInBytes(queue);

        return 0;
    }

/* ---------------- Other Methods ---------------- */
char* CBQ_strIntoHeap(const char* str)
{
    char* buff;
    size_t size = 0;

    while (str[size])
        size++;

    buff = (char*) CBQ_MALLOC(sizeof (char) * size);

    while (size--)
        buff[size] = str[size];

    return buff;
}

/* Debug */

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

    printf("Queue scheme:\n");
    for (size_t i = 0; i < trustedQueue->size; i++) {

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
        for (size_t i = 0; i < trustedQueue->size; i++) {
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

/* ---------------- Callback Methods ---------------- */

/* push CB after delay, like JS func. Needs time.h lib */
int CBQ_SetTimeout(CBQueue_t* queue, clock_t delay, int isSec,
    CBQueue_t* targetQueue, QCallback func, unsigned int vParamc, CBQArg_t* vParams)
{
    clock_t targetTime;
    int retst = 0;

    BASE_ERR_CHECK(queue);
    if (targetQueue != queue) {
        BASE_ERR_CHECK(targetQueue);
    }

    if (isSec)
        targetTime = clock() + (delay * CLOCKS_PER_SEC);
    else
        targetTime = clock() + delay;

    retst = CBQ_Push(queue, CBQ_setTimeoutFrame__, vParamc, vParams, ST_ARG_C,
        (CBQArg_t) {.qVar = queue},
        (CBQArg_t) {.uiVar = targetTime},
        (CBQArg_t) {.qVar = targetQueue},
        (CBQArg_t) {.fVar = func});

    return retst;
}

int CBQ_setTimeoutFrame__(int argc, CBQArg_t* args)
{
    int retst = 0;

    if (clock() >= (clock_t) args[ST_DELAY].uiVar) {

        if (args[ST_QUEUE].qVar == args[ST_TRG_QUEUE].qVar)
            retst = args[ST_FUNC].fVar(argc - ST_ARG_C, args + ST_ARG_C);
        else
            retst = CBQ_Push(args[ST_TRG_QUEUE].qVar, args[ST_FUNC].fVar,
                        argc - ST_ARG_C, args + ST_ARG_C, 0, CBQ_NO_STPARAMS);

    } else
        retst = CBQ_Push(args[ST_QUEUE].qVar, CBQ_setTimeoutFrame__,
            argc - ST_ARG_C, (argc - ST_ARG_C)? args + ST_ARG_C : NULL, ST_ARG_C,
            (CBQArg_t) {.qVar = args[ST_QUEUE].qVar},
            (CBQArg_t) {.uiVar = args[ST_DELAY].uiVar},
            (CBQArg_t) {.qVar = args[ST_TRG_QUEUE].qVar},
            (CBQArg_t) {.fVar = args[ST_FUNC].fVar});

    return retst;
}
