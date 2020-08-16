#ifndef CBQCALLBACKS_H
#define CBQCALLBACKS_H

#include "cbqbuildconf.h"
#include "cbqueue.h"
#include "cbqlocal.h"

/* push CB after delay, like JS func */
int CBQ_SetTimeout(CBQueue_t* queue, CBQTicks_t delay, const int isSec,
    CBQueue_t* targetQueue, QCallback func, unsigned int vParamc, CBQArg_t* vParams);

#endif // CBQCALLBACKS_H
