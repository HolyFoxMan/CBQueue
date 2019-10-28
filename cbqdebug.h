#ifndef CBQDEBUG_H
#define CBQDEBUG_H

/* set that macros define to activate
 debug mode before including cbq headers */

    // #define CBQ_DEBUG

    #ifdef CBQ_DEBUG

        #include <stdio.h>

        /* allow Assert-like macros ASRT(EXP, STR) */
        #define CBQD_ASRT

        /* allow scheme of current queue state which is drawn in console
         * after push, exec and changing size methods.
         */
        #define CBQD_SCHEME

        /* Log which is printed after success execution of methods:
         *
         */
        #define CBQD_OUTPUTLOG

        /* Allow function with status outputing of
         * debug subsystems
         */
        #define CBQD_STATUS

    #endif // CBQ_DEBUG


    #ifdef CBQD_ASRT
        /* Macro Func which add quotes for string definition */
        #ifndef MVAL_TO_STR
            #define _MVAL_TO_STR(_VAL_) #_VAL_
            #define MVAL_TO_STR(_VAL_) _MVAL_TO_STR(_VAL_)
        #endif // MVAL_TO_STR
    #endif

    #ifdef CBQD_ASRT
        #define ASRT(EXP, STR) \
        {   \
            int st = (EXP); \
            if (!!(st)) { \
                if ((STR)[0]) \
                    printf("----------------\nError\nMessage \"%s\"\nReturned code %d\nBy expression\t%s\nIn file\t%s\nOn line %d\n\n", \
                    STR, st, MVAL_TO_STR(EXP),MVAL_TO_STR(__FILE__),__LINE__); \
                    else \
                    printf("----------------\nError\nReturned code %d\nBy expression %s\tIn file %s\nOn line %d\n\n", \
                    st, MVAL_TO_STR(EXP),MVAL_TO_STR(__FILE__),__LINE__); \
            } \
        }
    #else
        #define ASRT(EXP, STR) \
            (EXP);
    #endif

    #ifdef CBQD_STATUS

        void CBQ_outDebugSysStatus__(void);

        #define CBQ_OUTDEBUGSTATUS() \
            CBQ_outDebugSysStatus__()

    #else

        #define CBQ_OUTDEBUGSTATUS() \
            ((void)0)

    #endif // CBQD_STATUS


    #ifdef CBQD_SCHEME
    /* use only macros function CBQ_DRAWSCHEME(), not that method */
    int CBQ_drawScheme_chk__(void* queue);

    #define CBQ_DRAWSCHEME(P_QUEUE) \
        CBQ_drawScheme_chk__((void*)P_QUEUE)

    #else

        #define CBQ_DRAWSCHEME(P_QUEUE) \
            ((void)0)

    #endif // CBQD_SCHEME

#endif // CBQDEBUG_H
