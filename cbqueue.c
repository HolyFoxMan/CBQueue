#include "cbqlocal.h"

/* ---------------- local methods declaration ---------------- */
/* Container Methods */
static int CBQ_reallocCapacity__(CBQueue_t*, size_t);
static int CBQ_orderingDividedSegs__(CBQueue_t*, size_t*);
static int CBQ_orderingDividedSegsInFullQueue__(CBQueue_t*);
static void CBQ_containersSwapping__(MAY_REG CBQContainer_t*, MAY_REG CBQContainer_t*, MAY_REG size_t, const int);
static void CBQ_containersCopy__(MAY_REG const CBQContainer_t *restrict, MAY_REG CBQContainer_t *restrict, MAY_REG size_t);
static int CBQ_containersRangeInit__(CBQContainer_t*, unsigned int, size_t, const int);
static void CBQ_containersRangeFree__(MAY_REG CBQContainer_t*, MAY_REG size_t);
static void CBQ_incIterCapacityChange__(CBQueue_t*, const int);
static int CBQ_getIncIterVector__(CBQueue_t* trustedQueue);
static int CBQ_incCapacity__(CBQueue_t*, size_t, const int);
static int CBQ_decCapacity__(CBQueue_t*, size_t, const int);
/* Arguments */
static int CBQ_changeArgsCapacity__(CBQContainer_t*, unsigned int, const int);
static void CBQ_copyArgs__(MAY_REG const CBQArg_t *restrict, MAY_REG CBQArg_t *restrict, MAY_REG unsigned int);
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
        printf("Notice: %s\n", STR), fflush(stdout)

#else

    #define CBQ_MSGPRINT(STR) \
        ((void)0)

#endif // CBQD_OUTPUTLOG

////////////////////////////////////////////////////////////////////////////////
//                                                                            //

/* ---------------- Base methods ---------------- */
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
    OPT_BASE_ERR_CHECK(queue);

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

int CBQ_QueueCopy(CBQueue_t* dest, CBQueue_t* src)
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
            for (int j = i - 1; j >= 0; j--)
                CBQ_MEMFREE(tmpCoArr[j].args);
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

int CBQ_QueueConcat(CBQueue_t* dest, CBQueue_t* src)
{

    return 0;
}

int CBQ_ChangeCapacity(CBQueue_t* queue, const int changeTowards, size_t customNewCapacity, const int adaptCCapacityByLimits)
{
    size_t errSt;

    BASE_ERR_CHECK(queue);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    CBQ_MSGPRINT("Queue capacity changing...");

    /* by selected mode with auto-dec/inc params */
    if (changeTowards == CBQ_INC_CAPACITY) {
        errSt = CBQ_incCapacity__(queue, 0, 1);
        if (errSt)
            return errSt;

    } else if (changeTowards == CBQ_DEC_CAPACITY) {
        errSt = CBQ_decCapacity__(queue, 0, 1);
        if (errSt)
            return errSt;

    /* by new custom capacity */
    } else {
        if (customNewCapacity <= 0 || customNewCapacity > CBQ_QUEUE_MAX_CAPACITY)
            return CBQ_ERR_ARG_OUT_OF_RANGE;

        ssize_t delta = (ssize_t) (customNewCapacity - queue->capacity);

        if (!delta)
            return CBQ_ERR_CUR_CH_CAPACITY_NOT_AFFECT;
        /* inc */
        else if (delta > 0) {
            errSt = CBQ_incCapacity__(queue, delta, adaptCCapacityByLimits);
            if (errSt)
                return errSt;
        /* dec */
        } else {
            errSt = CBQ_decCapacity__(queue, -delta, adaptCCapacityByLimits);
            if (errSt)
                return errSt;
        }
    }

    CBQ_MSGPRINT("Queue capacity is changed");
    return 0;
}


int CBQ_ChangeIncCapacityMode(CBQueue_t* queue, int newIncCapacityMode, size_t newMaxCapacityLimit, const int tryToAdaptCapacity, const int adaptMaxCapacityLimit)
{
    int errSt;

    OPT_BASE_ERR_CHECK(queue);

    #ifndef NO_EXCEPTIONS_OF_BUSY
    if (queue->execSt == CBQ_EST_EXEC)
        return CBQ_ERR_IS_BUSY;
    #endif // NO_EXCEPTIONS_OF_BUSY

    CBQ_MSGPRINT("Queue capacity mode changing...");

    if (newIncCapacityMode != CBQ_SM_LIMIT) {
        queue->maxCapacityLimit = 0;
        queue->incCapacityMode = newIncCapacityMode;
        return 0;
    } else {
        if (newMaxCapacityLimit < CBQ_QUEUE_MIN_CAPACITY || newMaxCapacityLimit > CBQ_QUEUE_MAX_CAPACITY)
            return CBQ_ERR_ARG_OUT_OF_RANGE;

        /* capacity does not fit into new limits */
        if (newMaxCapacityLimit < queue->capacity) {

            if (tryToAdaptCapacity) {
                CBQ_MSGPRINT("Queue capacity is adapt...");
                errSt = CBQ_decCapacity__(queue, queue->capacity - newMaxCapacityLimit, adaptMaxCapacityLimit);
                /* without capacity max limit adaptation flag the function may just give an error and close */
                if (errSt)
                    return errSt;

                /* if it was not possible to equalize even after capacity decrementing */
                if (newMaxCapacityLimit != queue->capacity)
                    newMaxCapacityLimit = queue->capacity;
            }
            else if (adaptMaxCapacityLimit)
                newMaxCapacityLimit = queue->capacity;
            else
                return CBQ_ERR_CAPACITY_NOT_FIT_IN_LIMIT;
        }

        queue->incCapacityMode = CBQ_SM_LIMIT;
        queue->maxCapacityLimit = newMaxCapacityLimit;
        queue->incCapacity = INIT_INC_CAPACITY;
    }

    CBQ_MSGPRINT("Queue capacity mode is changed");
    return 0;
}

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
    size_t ptr;
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

    for (ptr = queue->rId; ptr != queue->sId; ptr = (ptr + 1) % queue->capacity) { // % helps to loop ptr into capacity frames

        container = queue->coArr + ptr;

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

    for (ptr = queue->sId; ptr != queue->rId; ptr = (ptr + 1) % queue->capacity) {

        container = queue->coArr + ptr;

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

/* ---------------- Container methods ---------------- */
int CBQ_containersRangeInit__(CBQContainer_t* coFirst, unsigned int iniArgCap, size_t len, const int restore_pos_fail)
{
    MAY_REG CBQContainer_t* container = coFirst;
    MAY_REG size_t remLen = len;
    int errSt = 0;

    do {
        /* Container init */
        *container = (CBQContainer_t) {
            .func = NULL,
            .capacity = iniArgCap,
            .argc = 0

            #ifdef CBQD_SCHEME
            , .label = '-'
            #endif // CBQD_SCHEME
        };
        container->args = (CBQArg_t*) CBQ_MALLOC(sizeof(CBQArg_t) * iniArgCap);
        if (container->args == NULL) {
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

int CBQ_incCapacity__(CBQueue_t* trustedQueue, size_t delta, const int alignToMaxCapacityLimit)
{
    int errSt;
    int usedGeneratedIncrement;
    size_t remainder;

    /* in static mode, the capacity cannot increase */
    if (trustedQueue->incCapacityMode == CBQ_SM_STATIC)
        return CBQ_ERR_STATIC_CAPACITY_OVERFLOW;

    /* Get generated delta */
    if (!delta) {
        usedGeneratedIncrement = 1;
        delta = trustedQueue->incCapacity;
    }
    else
        usedGeneratedIncrement = 0;

    /* Checking the delta */
    remainder = (size_t) (trustedQueue->incCapacityMode == CBQ_SM_LIMIT?
                        trustedQueue->maxCapacityLimit :
                        CBQ_QUEUE_MAX_CAPACITY) - trustedQueue->capacity;

    if (remainder < delta) {
        /* if limit has been reached or the delta cannot be aligned */
        if (!remainder || !alignToMaxCapacityLimit) {
            if (trustedQueue->incCapacityMode == CBQ_SM_LIMIT)
                return CBQ_ERR_LIMIT_CAPACITY_OVERFLOW;
            else
                return CBQ_ERR_MAX_CAPACITY_OVERFLOW;
        } else
            delta = remainder;
    }

    /* Ordering cells */
    if (!trustedQueue->sId) {
        if (trustedQueue->status != CBQ_ST_EMPTY)
            trustedQueue->sId = trustedQueue->capacity;
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
        trustedQueue->sId = trustedQueue->capacity;
    }

    /* Realloc mem to new capacity */
    errSt = CBQ_reallocCapacity__(trustedQueue, trustedQueue->capacity + delta);
    if (errSt)
        return errSt;

    /* Init new allocated containers */
    errSt = CBQ_containersRangeInit__(trustedQueue->coArr + trustedQueue->capacity, trustedQueue->initArgCap, delta, REST_MEM);
    if (errSt) {

        if (errSt != CBQ_ERR_MEM_BUT_RESTORED)
            return errSt;

        if (CBQ_reallocCapacity__(trustedQueue, trustedQueue->capacity))
            return CBQ_ERR_MEM_ALLOC_FAILED; // totally failed

        if (trustedQueue->sId == trustedQueue->capacity)
            trustedQueue->sId = 0; // if in the past the queue was full

        return errSt;
    }

    if (usedGeneratedIncrement)
        CBQ_incIterCapacityChange__(trustedQueue, CBQ_getIncIterVector__(trustedQueue));

    /* Sets new incremented capacity and status */
    trustedQueue->capacity += delta;

    if (trustedQueue->status == CBQ_ST_FULL)
        trustedQueue->status = CBQ_ST_STABLE;

    CBQ_MSGPRINT("Queue capacity incremented");
    CBQ_DRAWSCHEME_IN(trustedQueue);

    return 0;
}

void CBQ_incIterCapacityChange__(CBQueue_t* trustedQueue, const int direction)
{
    if (direction)  { // Up
        trustedQueue->incCapacity <<= 1;    // * 2
        if (trustedQueue->incCapacity > MAX_INC_CAPACITY)
            trustedQueue->incCapacity = MAX_INC_CAPACITY;
    } else {    // Down
        trustedQueue->incCapacity >>= 1;
        if (trustedQueue->incCapacity < MIN_INC_CAPACITY)
            trustedQueue->incCapacity = MIN_INC_CAPACITY;
    }
}

/* For CBQ_SM_LIMIT and CBQ_QUEUE_MAX_CAPACITY mods */
int CBQ_getIncIterVector__(CBQueue_t* trustedQueue)
{
    if (trustedQueue->incCapacityMode == CBQ_SM_LIMIT) {
        if (trustedQueue->maxCapacityLimit - trustedQueue->capacity > trustedQueue->capacity)
            return 1;   // Up
        else
            return 0;   // Down
    } else {
        if (CBQ_QUEUE_MAX_CAPACITY - trustedQueue->capacity > trustedQueue->capacity)
            return 1;
        else
            return 0;
    }
}

int CBQ_decCapacity__(CBQueue_t* trustedQueue, size_t delta, const int alignToUsedCells)
{
    int errSt;
    size_t size;
    ssize_t remainder;

    /* Check and align delta */
    CBQ_GetSize(trustedQueue, &size);

    if (!delta)
        delta = trustedQueue->capacity - size;
    else if (delta > CBQ_QUEUE_MAX_CAPACITY)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    remainder = (ssize_t) trustedQueue->capacity - (ssize_t) (size > CBQ_QUEUE_MIN_CAPACITY? size : CBQ_QUEUE_MIN_CAPACITY);
    if (!remainder)
        return CBQ_ERR_CUR_CH_CAPACITY_NOT_AFFECT;

    remainder -= delta;
    if (remainder < 0) {
        if (alignToUsedCells)
            delta += remainder; // to delta balance
        else
            return CBQ_ERR_ENGCELLS_NOT_FIT_IN_NEWCAPACITY;
    }

    /* Offset or ordeing cells (for 4 cases)*/
    /* ---b--- -> b------ */
    if (trustedQueue->status == CBQ_ST_EMPTY)
        trustedQueue->rId = trustedQueue->sId = 0;
    else if (trustedQueue->rId) {

    /* s--r+++ -> ---r+++s */
        if (!trustedQueue->sId)
            trustedQueue->sId = trustedQueue->capacity;

    /* --r++s- -> r++s---   (if for example delta == 3, capacity - sId == 2) */
        if (trustedQueue->rId < trustedQueue->sId) {
            if (trustedQueue->capacity - trustedQueue->sId < delta) {
                CBQ_containersSwapping__(trustedQueue->coArr + (trustedQueue->rId), trustedQueue->coArr, size, 0);
                trustedQueue->rId = 0;
            }
    /* ++s--r+ -> r+++s-- */
        } else {
            errSt = CBQ_orderingDividedSegs__(trustedQueue, NULL);
            if (errSt)
                return errSt;
            trustedQueue->rId = 0;
        }
    }

    /* Free unused container args */
    CBQ_containersRangeFree__(trustedQueue->coArr + (trustedQueue->capacity - delta), delta);

    /* Mem reallocation */
    errSt = CBQ_reallocCapacity__(trustedQueue, trustedQueue->capacity - delta);
    if (errSt) {    // mem alloc error

        #if REST_MEM == 1
        int errStRest = 0;
        errStRest = CBQ_containersRangeInit__(trustedQueue->coArr, trustedQueue->initArgCap, delta, 1);
        if (!errStRest)
            return CBQ_ERR_MEM_BUT_RESTORED;
        #endif // REST_MEM

        return errSt;
    }

    CBQ_incIterCapacityChange__(trustedQueue, 0); // when reducing the capacity, it is logical to reduce the incCapacity var

    /* Sets new capacity and sId */
    trustedQueue->capacity -= delta;
    trustedQueue->sId = trustedQueue->rId + size;
    if (trustedQueue->sId == trustedQueue->capacity)
        trustedQueue->sId = 0;

    if (!remainder) // no free cells left
        trustedQueue->status = CBQ_ST_FULL;

    CBQ_MSGPRINT("Queue capacity decremented");
    CBQ_DRAWSCHEME_IN(trustedQueue);

    return 0;
}

int CBQ_reallocCapacity__(CBQueue_t* trustedQueue, size_t newCapacity)
{
    void* reallocp; // for safety old data

    reallocp = CBQ_REALLOC(trustedQueue->coArr, sizeof(CBQContainer_t) * newCapacity);
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
int CBQ_orderingDividedSegs__(CBQueue_t* trustedQueue, size_t* sizeP)
{
    CBQContainer_t* tmpCoArr;
    size_t size;

    CBQ_GetSize(trustedQueue, &size); // as new sId pos

    tmpCoArr = (CBQContainer_t*) CBQ_MALLOC(sizeof(CBQContainer_t) * size);
    if (tmpCoArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* copy cells between rId and last cell */
    CBQ_containersCopy__(trustedQueue->coArr + (trustedQueue->rId), tmpCoArr, trustedQueue->capacity - trustedQueue->rId);
    /* copy cells before sId into next free cells */
    CBQ_containersCopy__(trustedQueue->coArr, tmpCoArr + (trustedQueue->capacity - trustedQueue->rId), trustedQueue->sId);
    /* copy space of free cells to end of array with swap in reverse (to avoid data overlay) */
    CBQ_containersSwapping__(trustedQueue->coArr + (trustedQueue->rId - 1), // at last free cell
                             trustedQueue->coArr + (trustedQueue->capacity - 1), trustedQueue->capacity - size, 1);
    /* copy to start array from temp */
    CBQ_containersCopy__(tmpCoArr, trustedQueue->coArr, size);

    CBQ_MEMFREE(tmpCoArr);

    if (sizeP)
        *sizeP = size;
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
    size_t capacityOfSegment;

    tmpCoArr = (CBQContainer_t*) CBQ_MALLOC(sizeof(CBQContainer_t) * trustedQueue->sId);
    if (tmpCoArr == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    /* copy cells before sId into temp */
    CBQ_containersCopy__(trustedQueue->coArr, tmpCoArr, trustedQueue->sId);
    capacityOfSegment = trustedQueue->capacity - trustedQueue->rId; // get capacity of second part
    /* swap cells with second part */
    CBQ_containersSwapping__(trustedQueue->coArr + (trustedQueue->rId), trustedQueue->coArr, capacityOfSegment, 0);
    /* copy first part into place after seconf part */
    CBQ_containersCopy__(tmpCoArr, trustedQueue->coArr + (capacityOfSegment), trustedQueue->sId);

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
void CBQ_containersSwapping__(MAY_REG CBQContainer_t* srcp, MAY_REG CBQContainer_t* destp, MAY_REG size_t len, const int reverse_iter)
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
int CBQ_changeArgsCapacity__(CBQContainer_t* container, unsigned int newCapacity, const int copyArgsData)
{
    void* reallocp;

    if (newCapacity > MAX_CAP_ARGS)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    if (copyArgsData)
        reallocp = CBQ_REALLOC(container->args, sizeof(CBQArg_t) * (size_t) newCapacity);
    else {
        CBQ_MEMFREE(container->args);   // free old mem alloc
        reallocp = CBQ_MALLOC(sizeof(CBQArg_t) * (size_t) newCapacity);
    }
    if (reallocp == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    container->args = (CBQArg_t*) reallocp;
    container->capacity = newCapacity;

    return 0;
}

void CBQ_copyArgs__(MAY_REG const CBQArg_t *restrict src, MAY_REG CBQArg_t *restrict dest, MAY_REG unsigned int len)
{
    do {
        *dest++ = *src++;
    } while (--len);
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
    container = queue->coArr + queue->sId;  // container = &queue->coArr[queue->sId]

    if (varParams) {

        if (varParamc > container->capacity) {

            CBQ_MSGPRINT("Auto inc arg capacity...");

            errSt = CBQ_changeArgsCapacity__(container, varParamc, 0);
            if (errSt)
                return errSt;
        }

        CBQ_copyArgs__(varParams, container->args, varParamc);
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
int CBQ_GetSize(CBQueue_t* queue, size_t* engSize)
{
    OPT_BASE_ERR_CHECK(queue);
    if (engSize == NULL)
        return CBQ_ERR_ARG_NULL_POINTER;

    if (queue->rId < queue->sId)
        *engSize =  queue->sId - queue->rId;
    else if (queue->status == CBQ_ST_FULL || queue->rId > queue->sId)
        *engSize = queue->capacity - queue->rId + queue->sId;
    else
        *engSize = 0;   // for empty queue

    return 0;
}

int CBQ_GetCapacityInBytes(CBQueue_t* queue, size_t* byteCapacity)
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

int CBQ_GetFullInfo(CBQueue_t* queue, int *restrict getStatus, size_t *restrict getCapacity, size_t *restrict getSize,
    int *restrict getIncCapacityMode, size_t *restrict getMaxCapacityLimit, size_t *restrict getCapacityInBytes)
    {
        OPT_BASE_ERR_CHECK(queue);

        if (getStatus)
            *getStatus = queue->status;

        if (getCapacity)
            *getCapacity = queue->capacity;

        if (getSize)
            CBQ_GetSize(queue, getSize);

        if (getIncCapacityMode)
            *getIncCapacityMode = queue->incCapacityMode;

        if (getMaxCapacityLimit)
            *getMaxCapacityLimit = queue->maxCapacityLimit;

        if (getCapacityInBytes)
            CBQ_GetCapacityInBytes(queue, getCapacityInBytes);

        return 0;
    }

int CBQ_GetVerIndex(void)
{
    const int verId =
    #ifdef GEN_VERID
        ((CBQ_CUR_VERSION & BYTE_MASK)? CBQ_CUR_VERSION & BYTE_MASK : 1)  // first byte
        #if !defined(CBQ_NO_DEBUG) && defined(CBQ_DEBUG)
        | 1 << (CBQ_VI_DEBUG + BYTE_OFFSET)
        #endif  // debug id
        #ifdef NO_BASE_CHECK
        | 1 << (CBQ_VI_NBASECHECK + BYTE_OFFSET)
        #endif // NO_BASE_CHECK
        #ifdef NO_EXCEPTIONS_OF_BUSY
        | 1 << (CBQ_VI_NEXCOFBUSY + BYTE_OFFSET)
        #endif // NO_EXCEPTIONS_OF_BUSY
        #ifdef NO_VPARAM_CHECK
        | 1 << (CBQ_VI_NVPARAMCHECK + BYTE_OFFSET)
        #endif // NO_VPARAM_CHECK
        #ifdef REG_CYCLE_VARS
        | 1 << (CBQ_VI_REGCYCLEVARS + BYTE_OFFSET)
        #endif // REG_CYCLE_VARS
        #ifdef NO_REST_MEM_FAIL
        | 1 << (CBQ_VI_NRESTMEMFAIL + BYTE_OFFSET)
        #endif // NO_REST_MEM_FAIL
        #ifdef NO_FIX_ARGTYPES
        | 1 << (CBQ_VI_NFIXARGTYPES + BYTE_OFFSET)
        #endif // NO_FIX_ARGTYPES
    #else // GEN_VERID
        (int) 0
    #endif
        ;

    return verId;
}

int CBQ_CheckVerIndexByFlag(int fInfoType)
{
    int verId;
    verId = CBQ_GetVerIndex();

    if (!verId)
        return CBQ_ERR_VI_NOT_GENERATED;

    if (fInfoType < 0 || fInfoType >= CBQ_VI_LAST_FLAG)
        return CBQ_ERR_ARG_OUT_OF_RANGE;
    else if (fInfoType == CBQ_VI_VERSION)
        return verId & BYTE_MASK; // First Byte
    else
        return 1 & verId >> (fInfoType + BYTE_OFFSET);
}

int CBQ_GetDifferencesVerIdMask(int comparedVerId)
{
    int verId;

    verId = CBQ_GetVerIndex();
    if (!verId)
        return CBQ_ERR_VI_NOT_GENERATED;

    if (!comparedVerId)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    verId ^= comparedVerId;
    if (verId & BYTE_MASK)  // have difference versions
        verId = (verId & ~BYTE_MASK) | 1; // replace the value of the first byte with logic true (one)

    return verId;
}

int CBQ_GetAvaliableFlagsRange(void)
{
    #ifdef GEN_VERID
    return CBQ_VI_LAST_FLAG;
    #else
    return CBQ_ERR_VI_NOT_GENERATED;
    #endif
}

int CBQ_IsCustomisedVersion(void)
{
    int verId;
    verId = CBQ_GetVerIndex();

    if (!verId)
        return CBQ_ERR_VI_NOT_GENERATED;

    return !!(verId >> BYTE_CAPACITY); // !! - convert to bool value
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
    for (size_t i = 0; i < trustedQueue->capacity; i++) {

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
        for (size_t i = 0; i < trustedQueue->capacity; i++) {
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
int CBQ_SetTimeout(CBQueue_t* queue, long delay, const int isSec,
    CBQueue_t* targetQueue, QCallback func, unsigned int vParamc, CBQArg_t* vParams)
{
    CBQTicks_t targetTime;

    BASE_ERR_CHECK(queue);
    if (targetQueue != queue) {
        BASE_ERR_CHECK(targetQueue);
    }

    if (isSec)
        targetTime = CBQ_CURTICKS() + (CBQTicks_t)(delay * CBQ_TIC_P_SEC);
    else
        targetTime = CBQ_CURTICKS() + (CBQTicks_t)delay;

    return CBQ_Push(queue, CBQ_setTimeoutFrame__, vParamc, vParams, ST_ARG_C,
        (CBQArg_t) {.qVar = queue},
        (CBQArg_t) {.liVar = targetTime},
        (CBQArg_t) {.qVar = targetQueue},
        (CBQArg_t) {.fVar = func});
}

int CBQ_setTimeoutFrame__(int argc, CBQArg_t* args)
{
    if (CBQ_CURTICKS() >= (CBQTicks_t) args[ST_DELAY].liVar) {

        if (args[ST_QUEUE].qVar == args[ST_TRG_QUEUE].qVar)
            return args[ST_FUNC].fVar(argc - ST_ARG_C, args + ST_ARG_C);
        else
            return CBQ_Push(args[ST_TRG_QUEUE].qVar, args[ST_FUNC].fVar,
                        argc - ST_ARG_C, args + ST_ARG_C, 0, CBQ_NO_STPARAMS);

    } else
        return CBQ_Push(args[ST_QUEUE].qVar, CBQ_setTimeoutFrame__,
            argc - ST_ARG_C, (argc - ST_ARG_C)? args + ST_ARG_C : NULL, ST_ARG_C,
            (CBQArg_t) {.qVar = args[ST_QUEUE].qVar},
            (CBQArg_t) {.liVar = args[ST_DELAY].liVar},
            (CBQArg_t) {.qVar = args[ST_TRG_QUEUE].qVar},
            (CBQArg_t) {.fVar = args[ST_FUNC].fVar});
}
