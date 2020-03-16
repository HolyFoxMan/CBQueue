#ifndef CBQTEST_H
#define CBQTEST_H

    /* activate support of size_t format for printing
    vars with size_t type. Use SZ_PRTF instead "%..."
    */

    #ifdef __MINGW32__
    	#define __USE_MINGW_ANSI_STDIO 1
    	#define SZ_PRTF "%llu"
    #elif defined(__linux__)
    	#define SZ_PRTF "%zd"
    #else
    	#define SZ_PRTF "%lu"
    #endif

    #include <stdio.h>
    #include <conio.h>
    #include "cbqueue.h"

    #define CBQ_T_EXPLORE_VERSION() \
        CBQ_T_VerIdInfo(CBQ_CUR_VERSION)

    /* Base tests */
    void CBQ_T_HelloWorld(void);
    void CBQ_T_ControlTest(void);
    void CBQ_T_BusyTest(void);
    void CBQ_T_Params(void);
    void CBQ_T_SetTimeout_AutoGame(void);
    void CBQ_T_SetTimeout(void);
    void CBQ_T_VerIdInfo(int);
    void CBQ_T_ArgsTest(void);
    void CBQ_T_CopyTest(void);

#endif // CBQTEST_H


