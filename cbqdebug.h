#ifndef CBQDEBUG_H
#define CBQDEBUG_H

    #ifdef __cplusplus
    extern "C" {
    #endif // __cpluspluss

    #ifndef CBQ_DEBUG

        #ifdef CBQD_ASRT
            #undef CBQD_ASRT
        #endif

        #ifdef CBQD_SCHEME
            #undef CBQD_SCHEME
        #endif

        #ifdef CBQD_OUTPUTLOG
            #define CBQD_OUTPUTLOG
        #endif

        #ifdef CBQD_STATUS
            #undef CBQD_STATUS
        #endif

    #else // CBQ_DEBUG is on
        #include <stdio.h>
    #endif

    // 1 method (concat)
    #ifdef CBQD_ASRT
        /* Macro Func which add quotes for string definition */
        #ifndef MVAL_TO_STR
            #define _MVAL_TO_STR(_VAL_) #_VAL_
            #define MVAL_TO_STR(_VAL_) _MVAL_TO_STR(_VAL_)
        #endif // MVAL_TO_STR
    #endif

    // 1 method
    #ifdef CBQD_ASRT
        #define ASRT(EXP, STR) \
        {   \
            int st = (int)(EXP); \
            if (!!(st)) { \
                if ((STR)[0]) \
                    printf("----------------\nError\nMessage \"%s\"\nReturned val: %d\nBy expression\t%s\nIn file\t%s\nOn line %d\n\n", \
                    STR, st, MVAL_TO_STR(EXP),MVAL_TO_STR(__FILE__),__LINE__); \
                    else \
                    printf("----------------\nError\nReturned val: %d\nBy expression %s\tIn file %s\nOn line %d\n\n", \
                    st, MVAL_TO_STR(EXP),MVAL_TO_STR(__FILE__),__LINE__); \
            } \
        }
    #else
        #define ASRT(EXP, STR) \
            (EXP);
    #endif

    // 2 method
    #ifdef CBQD_STATUS

        void CBQ_outDebugSysStatus__(void);

        #define CBQ_OUTDEBUGSTATUS() \
            CBQ_outDebugSysStatus__()

    #else

        #define CBQ_OUTDEBUGSTATUS() \
            ((void)0)

    #endif // CBQD_STATUS

    // 3 method
    #ifdef CBQD_SCHEME
        /* use only macro functions, not that methods */
        int CBQ_drawScheme_chk__(const void* queue);
        typedef struct CBQueue_t CBQueue_t;
        void CBQ_drawScheme__(CBQueue_t*);

        #define CBQ_DRAWSCHEME_IN(P_TRUSTED_QUEUE) \
            CBQ_drawScheme__(P_TRUSTED_QUEUE)

        #define CBQ_DRAWSCHEME(P_QUEUE) \
            CBQ_drawScheme_chk__((void*)P_QUEUE)

    #else

        #define CBQ_DRAWSCHEME(P_QUEUE) \
            ((void)0)

        #define CBQ_DRAWSCHEME_IN(P_TRUSTED_QUEUE) \
            ((void)0)

    #endif // CBQD_SCHEME

    // 4 method
    #ifdef CBQD_OUTPUTLOG

        #define CBQ_MSGPRINT(STR) \
            printf("Notice: %s\n", STR), fflush(stdout)

    #else

        #define CBQ_MSGPRINT(STR) \
            ((void)0)

    #endif // CBQD_OUTPUTLOG


    #ifdef __cplusplus
    }
    #endif // __cplusplus

#endif // CBQDEBUG_H
