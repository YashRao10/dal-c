/*
 * sc_common.h -- dal-c shared definitions
 *
 * Types and status codes common to every dal-c component. Freestanding-
 * friendly: depends only on <stdint.h> and <stdbool.h>, both explicitly
 * permitted by MISRA C:2012 (Dir 4.6 exception, Rule 21.x notwithstanding
 * for these two standard headers).
 *
 * dal-c is written to a DAL C coding style: no dynamic memory allocation,
 * no recursion, no unbounded loops, single point of exit per function
 * where practical, all caller state passed by pointer.
 */
#ifndef DAL_C_SC_COMMON_H
#define DAL_C_SC_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Status codes for dal-c functions that can fail. Success is zero so that
 * `if (sc_x(...) != SC_OK)` reads naturally. Values are fixed and must not
 * be renumbered -- callers and logs may store them.
 */
typedef enum
{
    SC_OK        = 0,  /* operation completed                                 */
    SC_ERR_NULL  = 1,  /* a required pointer argument was NULL                */
    SC_ERR_PARAM = 2,  /* an argument was outside its permitted range         */
    SC_ERR_FULL  = 3,  /* container has no space for the requested element    */
    SC_ERR_EMPTY = 4   /* container holds no element to return                */
} sc_status_t;

/*
 * Advisory attribute: a dropped status return from a fallible function is a
 * defect. Compilers that understand it will warn; others see nothing.
 * Deviation: MISRA C:2012 Rule 1.2 (language extensions) -- justified, the
 * attribute has no semantic effect and improves static detectability.
 */
#if defined(__GNUC__)
#define SC_NODISCARD __attribute__((warn_unused_result))
#else
#define SC_NODISCARD
#endif

#endif /* DAL_C_SC_COMMON_H */
