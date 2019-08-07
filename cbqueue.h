#ifndef CBQUEUE_H
#define CBQUEUE_H

    #include <stdlib.h>
    #include <stdio.h>
    #include <stdint.h>
    #include "cbqdebug.h"

    /* Maximum (unstable) size of queue */
    #define CBQ_QUEUE_MAX_SIZE  65536

    /* Turn on that define if dont want base queue check on following methods:
     * push
     * exec
     * change size
     */
    // #define NO_BASE_CHECK

    /* Macros for callback without arguments which set into 4 arg in CBQ_Exec function */
    #define CBQ_NO_ARGS \
        (CBQArg_t) {0}

    /* Union type has argument base element which contain
     * used variables for function calls in queue.
     */
    typedef union CBQArg_t CBQArg_t;
    union CBQArg_t {

        int32_t     iVar;                 // integer
        uint32_t    uiVar;                // unsigned integer
        uint8_t     tuiVar;               // tiny unsigned integer (byte)
        double      dVar;                 // double
        char        cVar;                 // char
        char*       sVar;                 // string
        void*       pVar;                 // pointer (need explicit type conversion before using)
        struct CBQueue_t*  qVar;          // pointer to queue in which can send new call
        int (*fVar)(size_t, CBQArg_t*);   // function pointer

    };

    /* Main structure of callback queue instance
     * Used for async function calls support.
     * Functions must been have specific declaration
     * as basic type QCallback. All properties cant be
     * modifited without method functions.
     */
    typedef struct CBQueue_t CBQueue_t;
    struct CBQueue_t {

        /* init status */
        int     initSt;

        /* containers */
        size_t  size;
        struct  CBQContainer_t* coArr;
        int     sizeMode;
        size_t  sizeMaxLimit;
        size_t  incSize;

        /* pointers */
        size_t  rId;
        size_t  sId;
        int     status;

        /* debug */
        #ifdef CBQD_SCHEME
        int curLetter;
        #endif // CBQD_SCHEME

    };

    /* Size mode have some states which selected by numbers:
     * CBQ_SM_STATIC - after getting FULL flag the push method return error;
     * CBQ_SM_LIMIT - size can be auto incremented, but after out of size limit
     * push method return error too. Limit of size can be defined by argument sizeLimit.
     * Maximum limit is defined in CBQ_QUEUE_MAX_SIZE const;
     * CBQ_SM_MAX - unstable mode where size is not checks by limit and FULL flag,
     * but anyway new size will be no more constant which is defined in CBQ_QUEUE_MAX_SIZE.
     * You may change that const, but possible risks of lack of memory.
     */
    enum {
        CBQ_SM_STATIC,
        CBQ_SM_LIMIT,
        CBQ_SM_MAX
    };

    /* Its can be chosed in sizeLimit, but you can set custom limit.
     * By choosing custom you may get error part if started queue size
     * will be bigger than size limit.
     */
    enum {
        CBQ_SI_TINY =           16,
        CBQ_SI_SMALL =          64,
        CBQ_SI_MEDIUM =         512,
        CBQ_SI_BIG =            2048,
        CBQ_SI_HUGE =           8192
    };

    /* Type func pointer for functions which must be called
     * in the queue after earlier stored calls. Function poiner
     * may have various arguments, what has been defined
     * CBQArg_t union type. The number of arguments issued at
     * first var - argc.
     */
    typedef int (*QCallback) (int argc, CBQArg_t* argv);

    /* List of possible return statuses by callback queue methods.
     * First CBQ_SUCCESSFUL is simple zero constant which is used
     * almost in all standart C libs.
     */
    enum {
        CBQ_SUCCESSFUL,
        CBQ_ERR_NOT_INITED,
        CBQ_ERR_ALREADY_INITED,
        CBQ_ERR_ARG_NULL_POINTER,
        CBQ_ERR_ARG_OUT_OF_RANGE,
        CBQ_ERR_MEM_ALLOC_FAILED,
        CBQ_ERR_STATIC_SIZE_OVERFLOW,
        CBQ_ERR_LIMIT_SIZE_OVERFLOW,
        CBQ_ERR_MAX_SIZE_OVERFLOW,
        CBQ_ERR_QUEUE_IS_EMPTY
    };

    /* These enums choose in "changeTowards" param from ChangeSize method
     * CBQ_INC_SIZE - increments your size;
     * CBQ_DEC_SIZE - decrements your size;
     * CBQ_CUSTOM_SIZE - custom size which sets into "customNewSize" param.
     */
    enum {
        CBQ_CUSTOM_SIZE,
        CBQ_INC_SIZE,
        CBQ_DEC_SIZE,
    };

/* ---------------- Base methods ---------------- */
int CBQ_QueueInit(CBQueue_t* queue, size_t size, int sizeMode, size_t sizeMaxLimit);
int CBQ_QueueFree(CBQueue_t* queue);
int CBQ_ChangeSize(CBQueue_t* queue, int changeTowards, size_t customNewSize);

/* ---------------- Call Methods ---------------- */
int CBQ_Push(CBQueue_t* queue, QCallback func, int argc, CBQArg_t argv, ...);
int CBQ_Exec(CBQueue_t* queue, int* funcRetSt);
int CBQ_Clear(CBQueue_t* queue);

/* ---------------- Info Methods ---------------- */
#define CBQ_HAVECALL_P(TRUSTED_QUEUE_POINTER) \
    (!!(TRUSTED_QUEUE_POINTER)->status)

#define CBQ_GETSIZE_P(TRUSTED_QUEUE_POINTER) \
    (TRUSTED_QUEUE_POINTER)->size

#define CBQ_HAVECALL(TRUSTED_QUEUE) \
    (!!(TRUSTED_QUEUE).status)

#define CBQ_GETSIZE(TRUSTED_QUEUE) \
    (TRUSTED_QUEUE).size

size_t CBQ_GetCallAmount(CBQueue_t* queue);
int CBQ_GetFullInfo(CBQueue_t* queue, int* getStatus, size_t* getSize, size_t* getEngagedSize,
    int* getSizeMode, size_t* getSizeMaxLimit);
/* ---------------- Other Methods ---------------- */
char* CBQ_strIntoHeap(const char* str);
////////////////////////////////////////////////////////////////////////////////

#endif // CBQUEUE_H
