#include "cbqbuildconf.h"
#include "cbqdebug.h"
#include "cbqcapacity.h"
#include "cbqcontainer.h"

size_t CBQ_getSizeByIndexes__(const CBQueue_t* trustedQueue)
{
    if (trustedQueue->rId < trustedQueue->sId)
        return trustedQueue->sId - trustedQueue->rId;
    else if (trustedQueue->status == CBQ_ST_FULL || trustedQueue->rId > trustedQueue->sId)
        return trustedQueue->capacity - trustedQueue->rId + trustedQueue->sId;
    else
        return 0;   // empty trustedQueue
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
        if (!(remainder && alignToMaxCapacityLimit)) {
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
int CBQ_getIncIterVector__(const CBQueue_t* trustedQueue)
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
    size = CBQ_getSizeByIndexes__(trustedQueue);

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

    size = CBQ_getSizeByIndexes__(trustedQueue); // as new sId pos

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

int CBQ_ChangeCapacity(CBQueue_t* queue, const int changeTowards, size_t customNewCapacity, const int adaptByLimits)
{
    size_t errSt;

    OPT_BASE_ERR_CHECK(queue);

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
            errSt = CBQ_incCapacity__(queue, delta, adaptByLimits);
            if (errSt)
                return errSt;
        /* dec */
        } else {
            errSt = CBQ_decCapacity__(queue, -delta, adaptByLimits);
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
