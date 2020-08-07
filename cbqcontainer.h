#ifndef CBQCONTAINER_H
#define CBQCONTAINER_H

#include "cbqbuildconf.h"
#include "cbqdebug.h"
#include "cbqueue.h"
#include "cbqlocal.h"

typedef struct CBQContainer_t CBQContainer_t;
struct CBQContainer_t {

    QCallback       func;
    CBQArg_t*       args;
    unsigned int    capacity;
    unsigned int    argc;

    #ifdef CBQD_SCHEME
    int label;
    #endif

};

void CBQ_containersSwapping__(MAY_REG CBQContainer_t*, MAY_REG CBQContainer_t*, MAY_REG size_t, const int);
void CBQ_containersCopy__(MAY_REG const CBQContainer_t *restrict, MAY_REG CBQContainer_t *restrict, MAY_REG size_t);
int CBQ_containersRangeInit__(CBQContainer_t*, unsigned int, size_t, const int);
void CBQ_containersRangeFree__(MAY_REG CBQContainer_t*, MAY_REG size_t);
int CBQ_changeArgsCapacity__(CBQContainer_t*, unsigned int, const int);
void CBQ_copyArgs__(MAY_REG const CBQArg_t *restrict, MAY_REG CBQArg_t *restrict, MAY_REG unsigned int);


#endif // CBQCONTAINER_H
