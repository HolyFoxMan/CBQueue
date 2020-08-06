#include "cbqcallbacks.h"

static int CBQ_setTimeoutFrame__(int, CBQArg_t*);

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

static int CBQ_setTimeoutFrame__(int argc, CBQArg_t* args)
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
