#ifndef CBQLOCAL_H
#define CBQLOCAL_H

    #include "cbqueue.h"

    #if !defined(true) && !defined(false)
        #define true 1
        #define false 0
    #endif

    #define CBQ_QUEUE_MAX_SIZE  65536
    #define MIN_SIZE        1
    #define DEC_BUFF_PERC   1.1L

    #define INIT_INC_SIZE   8
    #define MAX_INC_SIZE    16384

    #define MAX_CO_ARG      20
    #define DEF_CO_ARG      5

    /* Getting bytes for header in save state data
     * Its  int:
            sizeMode,
            status,
            curLetter   (have -1, if is not debug mode)
     * and size_t:
            size,
            sizeMaxLimit,
            incSize,
            rId,
            sId
    */
    #define GET_BASE_QUEUE_HEADER() \
        sizeof (int) * 3 + sizeof (size_t) * 5


    /* Getting one container size
     * Its all included variables, except CBQArg_t pointer
     */
    #define GET_BASE_CONTAINER_SIZE() \
        sizeof (CBQContainer_t) - sizeof (CBQArg_t*)

    /* Getting size of arguments array in container */
    #define GET_CONTAINER_ARGS_SIZE(container) \
        container.argc * sizeof (CBQArg_t)

    /* Queue init status types */
    enum {
        CBQ_IN_INITED = 0x51494E49,
        CBQ_IN_FREE = 0x51465245
    };

    /* Base queue status types */
    enum {
        CBQ_ST_EMPTY,
        CBQ_ST_STABLE,
        CBQ_ST_FULL
    };

    /* Executing status */
    enum {
        CBQ_EST_NO_EXEC,
        CBQ_EST_EXEC
    };

    typedef struct CBQContainer_t CBQContainer_t;
    struct CBQContainer_t {

        QCallback   func;
        int         argMax;
        int         argc;
        CBQArg_t*   args;

        #ifdef CBQD_SCHEME
        int label;
        #endif

    };

    #define CBQ_ALLOC_METHODS 1
    #if CBQ_ALLOC_METHODS == 1     // POSIX

        #define CBQ_MALLOC(size) \
            malloc(size)
        #define CBQ_REALLOC(pointer, size) \
            realloc(pointer, size)
        #define CBQ_MEMFREE(pointer) \
            free(pointer)

    #endif

    #define SWAP_BY_TEMP(A, B, TEMP) \
    TEMP = B; \
    B = A; \
    A = TEMP

    #define BASE_ERR_CHECK(QUEUE) \
        if ((QUEUE) == NULL) \
            return CBQ_ERR_ARG_NULL_POINTER; \
        if ((QUEUE)->initSt != CBQ_IN_INITED) \
            return CBQ_ERR_NOT_INITED \

    #ifndef NO_BASE_CHECK
        #define OPT_BASE_ERR_CHECK(QUEUE) BASE_ERR_CHECK(QUEUE)
    #else
        #define OPT_BASE_ERR_CHECK(QUEUE) ((void)0)
    #endif // NO_BASE_CHECK

    /* SetTimeout defs */
    #define ST_ARG_C    4

    enum { ST_QUEUE, ST_DELAY, ST_TRG_QUEUE, ST_FUNC };

    /* storing 64 bit value in register */
    #ifdef REG_CYCLE_VARS
        #define MAY_REG register
    #else
        #define MAY_REG
    #endif

#endif // CBQLOCAL_H
