#include "cbqcontainer.h"

int CBQ_containersRangeInit__(CBQContainer_t* coFirst, unsigned int iniArgCap, size_t len, const int restore_pos_fail)
{
    MAY_REG CBQContainer_t* container = coFirst;
    MAY_REG size_t remLen = len;
    int errSt = 0;

    do {
        /* Container init */
        *container = (CBQContainer_t) {
            .func = NULL,
            .capacity = iniArgCap,
            .argc = 0

            #ifdef CBQD_SCHEME
            , .label = '-'
            #endif // CBQD_SCHEME
        };
        container->args = (CBQArg_t*) CBQ_MALLOC(sizeof(CBQArg_t) * iniArgCap);
        if (container->args == NULL) {
            errSt = CBQ_ERR_MEM_ALLOC_FAILED;
            break;
        }

        container++;
    } while (--remLen);

    if (errSt) {
        if (!restore_pos_fail)
            return CBQ_ERR_MEM_ALLOC_FAILED;

        CBQ_containersRangeFree__(coFirst, len - remLen);

        return CBQ_ERR_MEM_BUT_RESTORED;
    }

    return 0;
}

void CBQ_containersRangeFree__(MAY_REG CBQContainer_t* container, MAY_REG size_t len)
{
    do {
        CBQ_MEMFREE(container->args);
        container++;
    } while (--len);
}

/* Accelerated cycle
 * srcp, destp - pointers of areas in queue
 * srcp - point of source cells
 * destp - where are they moving
 * tmpCo - for swaping memory info (not pointer)
 */
/* POT_REG - potential register var, same as register, if 64 compile */
void CBQ_containersSwapping__(MAY_REG CBQContainer_t* srcp, MAY_REG CBQContainer_t* destp, MAY_REG size_t len, const int reverse_iter)
{
    CBQContainer_t tmpCo;

    if (reverse_iter)
        do {    // data swap (memory information)
            SWAP_BY_TEMP(*srcp, *destp, tmpCo);
            --srcp, --destp;
        } while (--len);
    else
        do {    // data swap (memory information)
            SWAP_BY_TEMP(*srcp, *destp, tmpCo);
            ++srcp, ++destp;
        } while (--len);
}

void CBQ_containersCopy__(MAY_REG const CBQContainer_t *restrict srcp, MAY_REG CBQContainer_t *restrict destp, MAY_REG size_t len)
{
    do {
        *destp++ = *srcp++;
    } while (--len);
}

/* ---------------- Args Methods ---------------- */
int CBQ_changeArgsCapacity__(CBQContainer_t* container, unsigned int newCapacity, const int copyArgsData)
{
    void* reallocp;

    if (newCapacity > MAX_CAP_ARGS)
        return CBQ_ERR_ARG_OUT_OF_RANGE;

    if (copyArgsData)
        reallocp = CBQ_REALLOC(container->args, sizeof(CBQArg_t) * (size_t) newCapacity);
    else {
        CBQ_MEMFREE(container->args);   // free old mem alloc
        reallocp = CBQ_MALLOC(sizeof(CBQArg_t) * (size_t) newCapacity);
    }
    if (reallocp == NULL)
        return CBQ_ERR_MEM_ALLOC_FAILED;

    container->args = (CBQArg_t*) reallocp;
    container->capacity = newCapacity;

    return 0;
}

void CBQ_copyArgs__(MAY_REG const CBQArg_t *restrict src, MAY_REG CBQArg_t *restrict dest, MAY_REG unsigned int len)
{
    do {
        *dest++ = *src++;
    } while (--len);
}
