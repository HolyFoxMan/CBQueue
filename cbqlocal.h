#ifndef CBQLOCAL_H
#define CBQLOCAL_H

    #ifndef CBQ_CUR_VERSION
        #error No lib version specified
    #endif

// Limits and inits values:

    #define CBQ_QUEUE_MAX_CAPACITY  (SIZE_MAX >> 1)
    /* init capacites written in cbqueue.h */
    #define CBQ_QUEUE_MIN_CAPACITY  1

    #define MIN_INC_CAPACITY        1
    #define INIT_INC_CAPACITY       8
    #define MAX_INC_CAPACITY        16384

    #define MIN_CAP_ARGS        2
    #define INIT_CAP_ARGS       5
    #define MAX_CAP_ARGS        20

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

    #define CBQ_ALLOC_METHODS 1
    #if CBQ_ALLOC_METHODS == 1     // POSIX

        #define CBQ_MALLOC(capacity) \
            malloc(capacity)
        #define CBQ_REALLOC(pointer, capacity) \
            realloc(pointer, capacity)
        #define CBQ_MEMFREE(pointer) \
            free(pointer)

    #else
        #error Unknown choosed mem alloc methods
    #endif

    #define CBQ_TIMER_METHODS 1
    #if CBQ_TIMER_METHODS == 1    // POSIX

        #define CBQ_CURTICKS() \
            ((CBQTicks_t)clock())

        #define CBQ_TIC_P_SEC \
            CLOCKS_PER_SEC

        typedef clock_t CBQTicks_t;

    #else // CBQ_TIMER_METHODS
        #error Unknown choosed timer methods
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

    #ifndef NO_REST_MEM_FAIL
        #define REST_MEM 1
    #else
        #define REST_MEM 0
    #endif

    #define BYTE_CAPACITY 8
    #define BYTE_OFFSET 7
    #define BYTE_MASK 0xFF

#endif // CBQLOCAL_H
