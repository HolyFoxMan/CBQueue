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

#endif // CBQLOCAL_H
