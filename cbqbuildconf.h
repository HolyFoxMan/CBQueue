#ifndef CBQBUILDCONF_H
#define CBQBUILDCONF_H

/* Build configuration file
 * Sets compile lib and debug features.
 * You may set these macros here or in compiler options for predefined macros.
 * !If you are using ready-made library, do not change flags in that file!
 * It is possible to view the set flags through special (version) functions.
 */

 /* ---------------- Lib features ---------------- */

/* Current version (could to compare by verId function).
 * Useful tip: use CBQ_T_EXPLORE_VERSION() from the cbqtest.h
 * to check the version of the library used.
 */
    #define CBQ_CUR_VERSION 2

/* Version 1 (initial):
 *  Queue struct;
 *  Argument union;
 *  Queue methods:
 *   Init, Clear, Free;
 *   Push (Static, variable params), PushOnlyVP (variable), PushVoid (No params);
 *   Exec, SetTimeout (JS-like);
 *   ChangeCapacity, ChangeIncCapacityMode,
 */
/* Version 2:
 *  Added queue copy, concatenation methods;
 *  CPP class-wrapper
 */

/* Macro flags */
/* Turn on that define if dont want base queue check on following methods:
 * push
 * exec
 * change capacity
 * set timeout
 */
// #define NO_BASE_CHECK

/* Disable exceptions, which can be obtained by queue methods (except Push, set timeout and info mehtods)
 * in callbacks which are processed from the same queue
 */
// #define NO_EXCEPTIONS_OF_BUSY

/* No dynamic args in push method check */
// #define NO_VPARAM_CHECK

/* Register vars in functions with cycle (copy data) */
// #define REG_CYCLE_VARS

/* Do not restore memory after unsuccessful allocation (There will be a memory leak) */
// #define NO_REST_MEM_FAIL

/* Disable stdint.h type declarations for CBQArg_t */
// #define NO_FIX_ARGTYPES

/* Enable to generate the identifier of the compiled library.
 * Possibly unsafe, because it stores embedded information about the enabled flags.
 */
    #define GEN_VERID

/* ---------------- Debug features ---------------- */

/* set that macro define to activate
 * debug mode, which allow to using bunch of debugging features
 * if that macro is not sets anywhere, second macro debug flags
 * will be ignored
 */
//    #define CBQ_DEBUG

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

#endif // CBQBUILDCONF_H
