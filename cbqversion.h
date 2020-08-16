#ifndef CBQVERSION_H
#define CBQVERSION_H

#include "cbqbuildconf.h"

/* VerId Information
 * You can just call CBQ_T_EXPLORE_VERSION() from cbqtest.h to get readable information of used lib
 */
enum CBQ_VI {  // flags for checking VerId by CBQ_CheckVerIndexByFlag function
    CBQ_VI_VERSION,
    CBQ_VI_DEBUG,       // starts after first byte
    CBQ_VI_NBASECHECK,
    CBQ_VI_NEXCOFBUSY,
    CBQ_VI_NVPARAMCHECK,
    CBQ_VI_REGCYCLEVARS,
    CBQ_VI_NRESTMEMFAIL,
    CBQ_VI_NFIXARGTYPES,

    CBQ_VI_LAST_FLAG    // use it only when comparing with the return value from the CBQ_GetAvaliableFlagsRange function
};

#define CBQ_ERR_VI_DETACH_NUM 255

enum CBQ_VI_Err {
    CBQ_ERR_VI_CATEGORY = CBQ_ERR_VI_DETACH_NUM,
    CBQ_ERR_VI_NOT_GENERATED,
    CBQ_ERR_VI_WRONG_CMP_VER_ID,
    CBQ_ERR_VI_UNKNOWN_FLAG
};

/* Returns identification value with version and used flags of current lib build.
 * If macro GEN_VERID has not been set, returns zero.
 */

int CBQ_GetVerIndex(void);
/* Return 1/0 or version by selected flag (CBQ_VI_...)*/
int CBQ_CheckVerIndexByFlag(int fInfoType);
/* Return logic mask (version differences are indicated by a 1 in the first byte) */
int CBQ_GetDifferencesVerIdMask(int comparedVerId);
/* May compare with CBQ_VI_LAST_FLAG value to avoid error in CBQ_CheckVerIndexByFlag.
 * But can returns error (CBQ_ERR_VI_NOT_GENERATED) if GEN_VERID has not been sets in used lib build.
 */
int CBQ_GetAvaliableFlagsRange(void);
/* Returns 1 if version was configured (At 0, you can hope for complete safety) */
int CBQ_IsCustomisedVersion(void);

#endif // CBQVERSION_H
