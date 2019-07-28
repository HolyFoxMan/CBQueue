#ifndef CBQTEST_H
#define CBQTEST_H

    /* activate support of "%llu" format for printing vars with size_t type */
    #define __USE_MINGW_ANSI_STDIO 1

    #include <stdio.h>
    #include <conio.h>
    #include "cbqueue.h"

    /* Base tests */
    void CBQ_T_HelloWorld(void);
    void CBQ_T_ControlTest(void);

#endif // CBQTEST_H


