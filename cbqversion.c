#include "cbqversion.h"
#include "cbqlocal.h"

int CBQ_GetVerIndex(void)
{
    const int verId =
    #ifdef GEN_VERID
        ((CBQ_CUR_VERSION & BYTE_MASK)? CBQ_CUR_VERSION & BYTE_MASK : 1)  // first byte
        #ifdef CBQ_DEBUG
        | 1 << (CBQ_VI_DEBUG + BYTE_OFFSET)
        #endif  // debug id
        #ifdef NO_BASE_CHECK
        | 1 << (CBQ_VI_NBASECHECK + BYTE_OFFSET)
        #endif // NO_BASE_CHECK
        #ifdef NO_EXCEPTIONS_OF_BUSY
        | 1 << (CBQ_VI_NEXCOFBUSY + BYTE_OFFSET)
        #endif // NO_EXCEPTIONS_OF_BUSY
        #ifdef NO_VPARAM_CHECK
        | 1 << (CBQ_VI_NVPARAMCHECK + BYTE_OFFSET)
        #endif // NO_VPARAM_CHECK
        #ifdef REG_CYCLE_VARS
        | 1 << (CBQ_VI_REGCYCLEVARS + BYTE_OFFSET)
        #endif // REG_CYCLE_VARS
        #ifdef NO_REST_MEM_FAIL
        | 1 << (CBQ_VI_NRESTMEMFAIL + BYTE_OFFSET)
        #endif // NO_REST_MEM_FAIL
        #ifdef NO_FIX_ARGTYPES
        | 1 << (CBQ_VI_NFIXARGTYPES + BYTE_OFFSET)
        #endif // NO_FIX_ARGTYPES

    #else // GEN_VERID
        (int) 0
    #endif
        ;

    return verId;
}

int CBQ_CheckVerIndexByFlag(int fInfoType)
{
    int verId;
    verId = CBQ_GetVerIndex();

    if (!verId)
        return CBQ_ERR_VI_NOT_GENERATED;

    if (fInfoType < 0 || fInfoType >= CBQ_VI_LAST_FLAG)
        return CBQ_ERR_VI_UNKNOWN_FLAG;
    else if (fInfoType == CBQ_VI_VERSION)
        return verId & BYTE_MASK; // First Byte
    else
        return 1 & verId >> (fInfoType + BYTE_OFFSET);
}

int CBQ_GetDifferencesVerIdMask(int comparedVerId)
{
    int verId;

    verId = CBQ_GetVerIndex();
    if (!verId)
        return CBQ_ERR_VI_NOT_GENERATED;

    if (!comparedVerId)
        return CBQ_ERR_VI_WRONG_CMP_VER_ID;

    verId ^= comparedVerId;

    if (verId & BYTE_MASK)  // have difference versions
        verId = (verId & ~BYTE_MASK) | 1; // replace the value of the first byte with logic true (one)

    return verId;
}

int CBQ_GetAvaliableFlagsRange(void)
{
    #ifdef GEN_VERID
    return CBQ_VI_LAST_FLAG;
    #else
    return CBQ_ERR_VI_NOT_GENERATED;
    #endif
}

int CBQ_IsCustomisedVersion(void)
{
    int verId;
    verId = CBQ_GetVerIndex();

    if (!verId)
        return CBQ_ERR_VI_NOT_GENERATED;

    return !!(verId >> BYTE_CAPACITY); // !! - convert to bool value
}
