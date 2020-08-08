#ifndef CBQCAPACITY_H
#define CBQCAPACITY_H

#include "cbqbuildconf.h"
#include "cbqueue.h"

int CBQ_incCapacity__(CBQueue_t*, size_t, const int);
int CBQ_decCapacity__(CBQueue_t*, size_t, const int);
int CBQ_reallocCapacity__(CBQueue_t*, size_t);
int CBQ_orderingDividedSegs__(CBQueue_t*, size_t*);
int CBQ_orderingDividedSegsInFullQueue__(CBQueue_t*);
void CBQ_incIterCapacityChange__(CBQueue_t*, const int);
int CBQ_getIncIterVector__(const CBQueue_t*);     // ret vector
size_t CBQ_getSizeByIndexes__(const CBQueue_t*);

#endif // CBQCAPACITY_H
