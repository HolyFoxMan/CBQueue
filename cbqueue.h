#ifndef CBQUEUE_H
#define CBQUEUE_H

    /* ---------------- C/C++ support ---------------- */
    #ifdef __cplusplus

        extern "C" {

        #define C_ATTR

        #include <cstdlib>
        #include <ctime>

    #else

        #define C_ATTR restrict

        #include <stdlib.h>
        #include <time.h>

        #if __STDC_VERSION__ < 199901L
            #error Needs minimum "c99" standart C version
        #endif

    #endif // __cplusplus

    /* ---------------- Build version control ---------------- */
    #define CBQ_MIN_VERSION 1
    #define CBQ_MAX_VERISON 2

    #ifndef CBQ_CUR_VERSION
        #define CBQ_CUR_VERSION MAX_VERSION
    #endif

    #if CBQ_CUR_VERSION < CBQ_MIN_VERSION || CBQ_CUR_VERSION > CBQ_MAX_VERISON
        #error CBQ_CUR_VERSION
    #endif


    #if CBQ_CUR_VERSION >= 2
        #define CBQ_ALLOW_V2_METHODS
    #endif

    /* ---------------- Argument structure declaration ---------------- */

    /* Union type has argument base element which contain
     * used variables for function calls in queue.
     * What to consider:
     * The C99 standard defines types with platform independent fixed capacity.
     * long is not more stably fixed, but it is also defined here.
     * It should also be remembered that unsigned floating point numbers
     * do not exist in C/C++, because because there are no analogues
     * of CPU commands.
     * A fixed capacity of 64 bit integers on a 32 bit machine will work,
     * but not in one machine instruction (which makes arithmetic slow).
     */
     #ifndef NO_FIX_ARGTYPES
        #include <stdint.h>
    #endif

    typedef union CBQArg_t CBQArg_t;
    union CBQArg_t {

    #ifdef NO_FIX_ARGTYPES
        unsigned char   utiVar;               // tiny unsigned integer (byte)
        unsigned short  usiVar;               // short int (machine word)
        unsigned int    uiVar;                // unsigned integer
        unsigned long long ulliVar;           // for maximum int value
        signed char     tiVar;
        signed short    siVar;
        signed int      iVar;
        signed long long lliVar;
    #else
        uint8_t         utiVar;
        uint16_t        usiVar;
        uint32_t        uiVar;
        uint64_t        ulliVar;
        int8_t          tiVar;
        int16_t         siVar;
        int32_t         iVar;
        int64_t         lliVar;
    #endif
        unsigned long   uliVar;
        signed long     liVar;
        size_t          szVar;                // size_t (unsigned)
        ssize_t         sszVar;               // signed size_t
        float           flVar;                // float
        double          dVar;                 // double
        char            cVar;                 // char
        char*           sVar;                 // string
        void*           pVar;                 // pointer (need explicit type conversion before using)
        struct CBQueue_t*  qVar;       // pointer to queue in which can send new call
        int (*fVar)(int, CBQArg_t*);   // function pointer
    };


    /* ---------------- Callback function declaration ---------------- */

    /* Type func pointer for functions which must be called
     * in the queue after earlier stored calls. Function poiner
     * may have various arguments, what has been defined
     * CBQArg_t union type. The number of arguments issued at
     * first var - argc.
     * If you do not use argc/argv, then with a strict check (-Wextra flag),
     * the compiler may issue a warning.
     * Add a UNUSED macro in the your callback declare in
     * the corresponding argument definition:
     *  int funCB(UNUSED int argc, CBQArg_t* args) or
     *  int funCB(int argc, CBQArg_t* args UNUSED)
     */
    #ifdef __GNUC__
        #define UNUSED __attribute((unused))
    #else
        #define UNUSED
    #endif

    /* Macro for callback without static parameters which set into 4 param in CBQ_Push function */
    #define CBQ_NO_STPARAMS \
        (CBQArg_t) {0}

    /* Macro for callback without scalable parameters, same as null macros */
    #define CBQ_NO_VPARAMS NULL

    typedef int (*QCallback) (int argc, CBQArg_t* args);


    /* ---------------- Queue (main) structure declaration ---------------- */

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

        /* exec status (only for exception catching) */
        #ifndef NO_EXCEPTIONS_OF_BUSY
        int     execSt;
        #endif // NO_EXCEPTIONS_OF_BUSY

        /* containers */
        struct  CBQContainer_t* C_ATTR coArr;
        size_t  capacity;
        size_t  maxCapacityLimit;
        size_t  incCapacity;
        int     incCapacityMode;
        unsigned int initArgCap;

        /* pointers */
        size_t  rId;
        size_t  sId;
        int     status;

        /* debug */
        #ifdef CBQD_SCHEME
        int curLetter;
        #endif // CBQD_SCHEME

    };

    /* Capacity mode have some states which selected by numbers:
     * CBQ_SM_STATIC - Queue capacity does not change. If we try to add a new call
     * to the filled queue, you will receive an appropriate signal (error).
     * CBQ_SM_LIMIT - capacity can be auto incremented, but after out of capacity limit
     * push method return error too. Limit of capacity can be defined by argument maxCapacityLimit.
     * (Maximum limit is defined in CBQ_QUEUE_MAX_CAPACITY const macro);
     * CBQ_SM_MAX - The maximum limit is sets by CBQ_QUEUE_MAX_CAPACITY, this macro is hidden.
     */
    enum CBQ_CapacityModes {
        CBQ_SM_STATIC,
        CBQ_SM_LIMIT,
        CBQ_SM_MAX
    };

    /* Its can be chosed in maxCapacityLimit, but you can set custom limit.
     * By choosing custom you may get error part if started queue capacity
     * will be bigger than CBQ_QUEUE_MAX_CAPACITY value.
     */
    enum CBQ_Capacities {
        CBQ_SI_TINY =           8,
        CBQ_SI_SMALL =          32,
        CBQ_SI_MEDIUM =         128,
        CBQ_SI_BIG =            1024,
        CBQ_SI_HUGE =           8192
    };


    /* ---------------- Queue functions declaration ---------------- */

    /* List of possible return error statuses by callback queue methods.
     * First CBQ_SUCCESSFUL is simple zero constant which is used
     * almost in all standart C libs.
     */
    enum CBQ_errors {
        CBQ_SUCCESSFUL,
        CBQ_ERR_NOT_INITED,
        CBQ_ERR_ALREADY_INITED,
        CBQ_ERR_ARG_NULL_POINTER,
        CBQ_ERR_ARG_OUT_OF_RANGE,
        CBQ_ERR_MEM_ALLOC_FAILED,
        CBQ_ERR_MEM_BUT_RESTORED,
        CBQ_ERR_STATIC_CAPACITY_OVERFLOW,
        CBQ_ERR_LIMIT_CAPACITY_OVERFLOW,
        CBQ_ERR_MAX_CAPACITY_OVERFLOW,
        CBQ_ERR_CUR_CH_CAPACITY_NOT_AFFECT,
        CBQ_ERR_ENGCELLS_NOT_FIT_IN_NEWCAPACITY,
        CBQ_ERR_CAPACITY_NOT_FIT_IN_LIMIT,
        CBQ_ERR_QUEUE_IS_EMPTY,
        CBQ_ERR_IS_BUSY,
        CBQ_ERR_VPARAM_VARIANCE,
        CBQ_ERR_HURTS_USED_ARGS,
        CBQ_ERR_INITCAP_IS_IDENTICAL,
        #ifdef CBQ_ALLOW_V2_METHODS
        CBQ_ERR_COUNT_NOT_FIT_IN_SIZE,
        CBQ_ERR_SAME_QUEUE,
        #endif
    };

    /* These enums choose in "changeTowards" param from ChangeCapacity method
     * CBQ_INC_CAPACITY - increments your capacity;
     * CBQ_DEC_CAPACITY - decrements your capacity;
     * CBQ_CUSTOM_CAPACITY - custom capacity which sets into "customNewCapacity" param.
     */
    enum CBQ_ChangeCapacityFlags {
        CBQ_CUSTOM_CAPACITY,
        CBQ_INC_CAPACITY,
        CBQ_DEC_CAPACITY
    };


/* ---------------- Base method sdeclaration ---------------- */
int CBQ_QueueInit(CBQueue_t* queue, size_t capacity, int incCapacityMode, size_t maxCapacityLimit, unsigned int customInitArgsCapacity);
int CBQ_Clear(CBQueue_t* queue);
int CBQ_QueueFree(CBQueue_t* queue);
#ifdef CBQ_ALLOW_V2_METHODS
int CBQ_QueueCopy(CBQueue_t* C_ATTR dest, const CBQueue_t* C_ATTR src);
int CBQ_QueueCorrectMove(CBQueue_t* C_ATTR dest, CBQueue_t* C_ATTR src);
int CBQ_QueueConcat(CBQueue_t* C_ATTR dest, const CBQueue_t* C_ATTR src);
int CBQ_QueueTransfer(CBQueue_t* C_ATTR dest, CBQueue_t* C_ATTR src, size_t count, const int cutByDestLimit, const int cutBySrcSize);
int CBQ_Skip(CBQueue_t* queue, size_t count, const int cutBySize, const int reverseOrder);
#endif // CBQ_CUR_VERSION


/* -------- Push/Execute methods declaration -------- */

/* macro function for pushing CB with static number (in runtime) of parameters
 * CBQ_PushStatic(&queue, callback, 3, (CBQArg_t) {.siVar = -10}, (CBQArg_t) {.uiVar = 20}, (CBQArg_t) {.cVar = '+'});
 * It�s not a very convenient call, therefore it�s recommended to create macros
 * with a specific number of arguments and name of functions.
 * For example:
 * CBQ_PushStatic(&(QUEUE), callbackSum, 3, CBQArg_t {.pVar = (PRESULT)}, CBQArg_t {.uiVar = (FIRST)}, (CBQArg_t) {.uiVar = (SECOND)});  -->
 * AsyncIntegerSum(QUEUE, &result, 113, 745);
 */
#define CBQ_PushStatic(queue, func, paramc, ...) \
    CBQ_Push(queue, func, 0, CBQ_NO_VPARAMS, paramc, __VA_ARGS__)

/*!Not for using */
#define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, count, ...) count

/* More convenient function call:
 * CBQ_PushN(&queue, callback, {1}, {.fVar = 0.5}, {.cVar = '/'});
 * where {1] - is abbreviated integer callback parameter.
 */
#define CBQ_PushN(queue, func, ...) \
    CBQ_PushOnlyVP(queue, func, GET_ARG_COUNT(__VA_ARGS__), (CBQArg_t[]) {__VA_ARGS__})

/* Alt macro push calls (not recommented) */
#define CBQ_PushN_(queue, func, ...) \
    CBQ_Push(queue, func, GET_ARG_COUNT(__VA_ARGS__), (CBQArg_t[]) {__VA_ARGS__}, 0, CBQ_NO_STPARAMS)
#define CBQ_PushVariable_(queue, func, paramc, pVarParamArray) \
    CBQ_Push(queue, func, paramc, pVarParamArray, 0, CBQ_NO_STPARAMS)


/* Set timeout macros funcs */
#define CBQ_SetTimeoutVoid(queue, delay, isSec, target_queue, func)  \
    CBQ_SetTimeout(queue, delay, isSec, target_queue, func, 0, CBQ_NO_VPARAMS)

/* self-push queue */
#define CBQ_SetTimeoutSP(queue, delay, isSec, func, vParamc, ...)  \
    CBQ_SetTimeout(queue, delay, isSec, queue, func, vParamc, __VA_ARGS__)

#define CBQ_SetTimeoutVoidSP(queue, delay, isSec, func)  \
    CBQ_SetTimeout(queue, delay, isSec, queue, func, 0, CBQ_NO_VPARAMS)

/* Method pushes callbacks by static and variadic passing of parameters (in run-time) */

int CBQ_Push(CBQueue_t* queue, QCallback func, unsigned int varParamc, CBQArg_t* varParams, unsigned int stParamc, CBQArg_t stParams, ...);
int CBQ_PushOnlyVP(CBQueue_t* queue, QCallback func, unsigned int varParamc, CBQArg_t* varParams);
int CBQ_PushVoid(CBQueue_t* queue, QCallback func);
int CBQ_Exec(CBQueue_t* queue, int* funcRetSt);
int CBQ_SetTimeout(CBQueue_t* queue, clock_t delay, const int isSec, CBQueue_t* targetQueue, QCallback func, unsigned int vParamc, CBQArg_t* vParams);

/* ---------------- Capacity changing methods declaration ---------------- */
int CBQ_ChangeCapacity(CBQueue_t* queue, const int changeTowards, size_t customNewCapacity, const int adaptByLimits);
int CBQ_ChangeIncCapacityMode(CBQueue_t* queue, int newIncCapacityMode, size_t newCapacityMaxLimit, const int tryToAdaptCapacity, const int adaptCapacityMaxLimit);
int CBQ_EqualizeArgsCapByCustom(CBQueue_t* queue, unsigned int customCapacity, const int passNonModifiableArgs);
int CBQ_ChangeInitArgsCapByCustom(CBQueue_t* queue, unsigned int customInitCapacity);

/* ---------------- Additional methods declaration ---------------- */
char* CBQ_strIntoHeap(const char* str);

/* ---------------- Info Methods declaration ---------------- */
#define CBQ_HAVECALL_P(TRUSTED_QUEUE_POINTER) \
    (!!(TRUSTED_QUEUE_POINTER)->status)

#define CBQ_GETCAPACITY_P(TRUSTED_QUEUE_POINTER) \
    (TRUSTED_QUEUE_POINTER)->capacity

#define CBQ_HAVECALL(TRUSTED_QUEUE) \
    (!!(TRUSTED_QUEUE).status)

#define CBQ_GETCAPACITY(TRUSTED_QUEUE) \
    (TRUSTED_QUEUE).capacity

#define CBQ_ISFULL_P(TRUSTED_QUEUE_POINTER) \
    ((TRUSTED_QUEUE_POINTER)->status == 2)

#define CBQ_ISFULL(TRUSTED_QUEUE) \
    ((TRUSTED_QUEUE).status == 2)

#define CBQ_ISEMPTY_P(TRUSTED_QUEUE_POINTER) \
    (!(TRUSTED_QUEUE_POINTER)->status)

#define CBQ_ISEMPTY(TRUSTED_QUEUE) \
    (!(TRUSTED_QUEUE).status)

int CBQ_GetSize(const CBQueue_t* queue, size_t* size);
int CBQ_GetCapacityInBytes(const CBQueue_t* queue, size_t* byteCapacity);

int CBQ_GetDetailedInfo( const CBQueue_t* queue,
    size_t *C_ATTR      getCapacity,
    size_t *C_ATTR      getSize,
    int    *C_ATTR      getIncCapacityMode,
    size_t *C_ATTR      getMaxCapacityLimit,
    size_t *C_ATTR      getCapacityInBytes
);

/* ---------------- End ---------------- */
#undef C_ATTR

    #ifdef __cplusplus
        }
    #endif // __cplusplus

#endif // CBQUEUE_H
