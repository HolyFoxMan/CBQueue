#ifndef CBQUEUE_H
#define CBQUEUE_H

    #ifdef __cplusplus
    extern "C" {
    #endif // __cplusplus

    /* At c99 */
    #if !defined(__cplusplus) && __STDC_VERSION__ < 199901L
        #error Needs "c99" version
    #endif

    #include <stdlib.h>
    #include <time.h>
    #include "cbqdebug.h"

    /* Maximum (unstable) size of queue is 65536 */

    /* ---------------- UNSAFETY MACROSES ---------------- */

    /* Turn on that define if dont want base queue check on following methods:
     * push
     * exec
     * change size
     * set timeout
     */
    // #define NO_BASE_CHECK

    /* Disable exceptions, which can be obtained by queue methods (except Push, set timeout and info mehtods)
     * in callbacks which are processed from the same queue
     */
    // #define NO_EXCEPTIONS_OF_BUSY

    /* No dynamic args in push method check */
    // #define NO_VPARAM_CHECK

    /* ---------------- UNSAFETY MACROSES ---------------- */

    /* Macros for callback without static parameters which set into 4 param in CBQ_Exec function */
    #define CBQ_NO_STPARAMS \
        (CBQArg_t) {0}

    /* Macros for callback without scalable parameters, same as null macros
    */
    #define CBQ_NO_VPARAMS NULL

    /* Union type has argument base element which contain
     * used variables for function calls in queue.
     */
    typedef union CBQArg_t CBQArg_t;
    union CBQArg_t {

        int             iVar;                 // integer
        unsigned int    uiVar;                // unsigned integer
        unsigned char   tuiVar;               // tiny unsigned integer (byte)
        size_t          szVar;                // size_t
        double          dVar;                 // double
        char            cVar;                 // char
        char*           sVar;                 // string
        void*           pVar;                 // pointer (need explicit type conversion before using)
        struct CBQueue_t*  qVar;          // pointer to queue in which can send new call
        int (*fVar)(int, CBQArg_t*);   // function pointer

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

        /* exec status (only for excpetion catching) */
        #ifndef NO_EXCEPTIONS_OF_BUSY
        int     execSt;
        #endif // NO_EXCEPTIONS_OF_BUSY

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
    typedef int (*QCallback) (int argc, CBQArg_t* args);

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
        CBQ_ERR_CUR_CH_SIZE_NOT_AFFECT,
        CBQ_ERR_QUEUE_IS_EMPTY,
        CBQ_ERR_IS_BUSY,
        CBQ_ERR_VPARAM_VARIANCE
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

/* -------- push macroses -------- */

/* macros function for pushing CB with static number (in runtime) of parameters */
#define CBQ_PushStatic(queue, func, paramc, ...) \
    CBQ_Push(queue, func, 0, CBQ_NO_VPARAMS, paramc, __VA_ARGS__)


/* macros function for variable passing of parameters */
#define CBQ_PushVariable(queue, func, paramc, pVarParamArray) \
    CBQ_Push(queue, func, paramc, pVarParamArray, 0, CBQ_NO_STPARAMS)

/* macros function for pushing CB without parameters */
#define CBQ_PushVoid(queue, func) \
    CBQ_Push(queue, func, 0, CBQ_NO_VPARAMS, 0, CBQ_NO_STPARAMS)

/* -------- set timeout macroses -------- */
#define CBQ_SetTimeoutVoid(queue, delay, isSec, target_queue, func)  \
    CBQ_SetTimeout(queue, delay, isSec, target_queue, func, 0, CBQ_NO_VPARAMS)

/* self-push queue */
#define CBQ_SetTimeoutSP(queue, delay, isSec, func, vParamc, ...)  \
    CBQ_SetTimeout(queue, delay, isSec, queue, func, vParamc, __VA_ARGS__)

#define CBQ_SetTimeoutVoidSP(queue, delay, isSec, func)  \
    CBQ_SetTimeout(queue, delay, isSec, queue, func, 0, CBQ_NO_VPARAMS)

/* Method pushes callbacks by static and variable passing of parameters (in run-time) */
int CBQ_Push(CBQueue_t* queue, QCallback func, int varParamc, CBQArg_t* varParams, int stParamc, CBQArg_t stParams, ...);
int CBQ_Exec(CBQueue_t* queue, int* funcRetSt);
int CBQ_SetTimeout(CBQueue_t* queue, clock_t delay, int isSec, CBQueue_t* targetQueue, QCallback func, int vParamc, CBQArg_t* vParams);

/* ---------------- Additional methods ---------------- */
int CBQ_ChangeSize(CBQueue_t* queue, int changeTowards, size_t customNewSize);
int CBQ_Clear(CBQueue_t* queue);
char* CBQ_strIntoHeap(const char* str);

/* Not used
int CBQ_SaveState(CBQueue_t* queue, unsigned char* data, size_t* receivedSize);
int CBQ_RestoreState(CBQueue_t* queue, unsigned char* data, size_t size);
*/

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

////////////////////////////////////////////////////////////////////////////////

    #ifdef __cplusplus
    }
    #endif // __cplusplus

#endif // CBQUEUE_H
