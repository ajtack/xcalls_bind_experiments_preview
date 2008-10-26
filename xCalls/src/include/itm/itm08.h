/* INTEL CONFIDENTIAL
 * Copyright 2006-2008 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material may contain trade secrets and proprietary and confidential
 * information of Intel Corporation and its suppliers and licensors,
 * and is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express
 * written permission.  No license under any patent, copyright, trade
 * secret or other intellectual property right is granted to or
 * conferred upon you by disclosure or delivery of the Materials,
 * either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must
 * be express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or
 * alter this notice or any other notice embedded in Materials by
 * Intel or Intel's suppliers or licensors in any way.
 */

/*
 * This file can be processed with doxygen to generate documentation.
 * If you do that you will get better results if you use this set of parameters
 * in your Doxyfile.
 *
 *  ENABLE_PREPROCESSING   = YES
 *  MACRO_EXPANSION        = YES
 *  EXPAND_ONLY_PREDEF     = YES
 *  PREDEFINED             = "_ITM_NORETURN(arg) = arg "\
 *                           "_ITM_CALL_CONVENTION= "   \
 *                           "_ITM_PRIVATE= "           \
 *                           "_ITM_EXPORT= "            \
 *  EXPAND_AS_DEFINED      = _ITM_FOREACH_TRANSFER
 */

/*!
 * \file itm08.h
 * \brief Header file for the SSG STM/HASTM interface.
 *
 * itm08.h contains the full 0.8 ABI interface between the compiler and the TM runtime.
 * Functions which can be called by user code are in itm_user.h.
 */

#if !defined(_ITM08_H)                            /* Idempotence */
# define _ITM08_H
//! Version string
# define _ITM_VERSION "0.82 (April 25 2008)"
/* We undef _ITM_VERSION_NO before defining it since it may have been set by the build system
 * to a less precise version than we know here. (e.g. 70 when we're 72).
 */
# undef _ITM_VERSION_NO
//! Version number (the version times 100)
# define _ITM_VERSION_NO 82

# ifdef __cplusplus
extern "C"
{
# endif                          /* __cplusplus */

# include "itm_stdint.h"
# include <xmmintrin.h>
# include <stddef.h>

    /* defined(_WIN32) should be sufficient, but it looks so much like a latent bug
     * that I also test _WIN64
     */
# if (defined (_WIN32) || defined (_WIN64))
#  define  _ITM_NORETURN(funcspec) __declspec(noreturn) funcspec
/* May need something here for Windows, but I'm not fixing that just now... */
#  define  _ITM_EXPORT
#  define  _ITM_PRIVATE
# else
#  define _ITM_NORETURN(funcspec) funcspec __attribute__((noreturn))
#  define _ITM_EXPORT  __attribute__((visibility("default")))
#  define _ITM_PRIVATE __attribute__((visibility("hidden" )))
# endif


# include "itm_user.h"

/* Opaque types passed over the interface.
 * In most cases these allow us to use pointers to specific opaque
 * types, rather than just "void *" and thus get some type checking
 * without having to make the content of the type visible.  This also
 * allows the data-flow from the result of one function to an argument
 * to another to be clearer.
 */
//! Opaque memento descriptor
struct _ITM_mementoS;
typedef struct _ITM_mementoS _ITM_memento;

/*! This is the same source location structure as is used for OpenMP. */
struct _ITM_srcLocationS
{
    int32_t reserved_1;
    int32_t flags;
    int32_t reserved_2;
    int32_t reserved_3;
    const char *psource; /**< psource contains a string describing the
     * source location generated like this
     * \code
     * sprintf(loc_name,";%s;%s;%d;%d;;", path_file_name, routine_name, sline, eline); 
     * \endcode 
     * where sline is the starting line and eline the ending line.
     */
};

typedef struct _ITM_srcLocationS _ITM_srcLocation;

/*! Values to describe properties of the code generated for a
 * transaction, passed in to startTransaction.
 */ 
typedef enum 
{
   pr_instrumentedCode   = 0x0001, /**< Instrumented code is available.  */
   pr_uninstrumentedCode = 0x0002, /**< Uninstrumented code is available.  */
   pr_multiwayCode = pr_instrumentedCode | pr_uninstrumentedCode,
   pr_hasNoXMMUpdate     = 0x0004, /**< Compiler has proved that no XMM registers are updated.  */
   pr_hasNoAbort         = 0x0008, /**< Compiler has proved that no user abort can occur. */
   pr_hasNoRetry         = 0x0010, /**< Compiler has proved that no user retry can occur. */
   pr_hasNoIrrevocable   = 0x0020, /**< Compiler has proved that the transaction does not become irrevocable. */
   pr_doesGoIrrevocable  = 0x0040, /**< Compiler has proved that the transaction always becomes irrevocable. */
   pr_hasNoSimpleReads   = 0x0080, /**< Compiler has proved that the transaction has no R<type> calls. */

   /* More detailed information about properties of instrumented code generation. 
    * These don't telll us that the transaction had no need for these barriers (like hasNoSimpleReads),
    * rather that the compiler simply generated native ld/st for these operations and omitted the
    * barriers. (Useful if the library implementation didn't need them, for instance an in-place
    * update STM never needs to know about "after Write" operations).
    */
   pr_aWBarriersOmitted  = 0x0100, /**< Compiler omitted "after write" barriers  */
   pr_RaRBarriersOmitted = 0x0200, /**< Compiler omitted "read after read" barriers  */
   pr_undoLogCode        = 0x0400, /**< Compiler generated only code for undo-logging, no other barriers. */
} _ITM_codeProperties;

/*! Result from startTransaction which describes what actions to take.  */
typedef enum
{
    a_runInstrumentedCode   = 0x01, /**< Run instrumented code.  */
    a_runUninstrumentedCode = 0x02, /**< Run uninstrumented code.  */
    a_saveLiveVariables     = 0x04, /**< Save live in variables (only legal with run instrumented code).  */
    a_restoreLiveVariables  = 0x08, /**< Restore live in variables (only legal with abort or retry).  */
    a_abortTransaction      = 0x10, /**< Branch to the abort label.  */
    a_retryTransaction      = 0x20, /**< Branch to the retry label.  */
} _ITM_actions;

/*! Values to be passed to changeTransactionMode. */
typedef enum 
{
    modeSerialIrrevocable, /**< Serial irrevocable mode. No abort, no retry. Good for legacy functions. */
    modeObstinate,         /**< Obstinate mode. Abort or retry are possible, but this transaction wins all contention fights. */
    modeOptimistic,        /**< Optimistic mode. Force transaction to run in optimistic mode. */
    modePessimistic,       /**< Pessimistic mode. Force transaction to run in pessimistic mode. */
} _ITM_transactionState;

/*! Values to be passed to _ITM_abort*Transaction */
typedef enum {
    userAbort = 1,     /**< __tm_abort was invoked. */
    userRetry = 2,     /**< __tm_retry was invoked. */              
    TMConflict= 4,     /**< The runtime detected a memory access conflict. */
    upLevelAbort = 8   /**< Abort transaction but don't jump back to it because we're unwinding the stack of
                          nested transactions */
} _ITM_abortReason;

/*! Version checking */
_ITM_EXPORT extern const char * _ITM_CALL_CONVENTION _ITM_libraryVersion (void);

/*! Call with the _ITM_VERSION_NO macro defined above, so that the
 * library can handle compatibility issues.
 */
_ITM_EXPORT extern int _ITM_CALL_CONVENTION _ITM_versionCompatible (int); /* Zero if not compatible */

/*! Initialization, finalization
 * Result of initialization function is zero for success, non-zero for failure, so you want
 * if (!_ITM_initializeProcess())
 * {
 *     ... error exit ...
 * }
 */
_ITM_EXPORT extern int  _ITM_CALL_CONVENTION _ITM_initializeProcess(void);      /* Idempotent */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_finalizeProcess(void);
_ITM_EXPORT extern int  _ITM_CALL_CONVENTION _ITM_initializeThread(void);       /* Idempotent */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_finalizeThread(void);

/*! Error reporting */
_ITM_NORETURN (_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_error (const _ITM_srcLocation *, int errorCode));

/* Transaction operations. */
/*! \param __msp a pointer through which the memento size is returned.
    \return a pointer to the current transaction descriptor.
 */
_ITM_EXPORT extern _ITM_transaction * _ITM_CALL_CONVENTION _ITM_getTransactionAndMementoSize (size_t *__msp); 

/* _ITM_getTransaction(void) and _ITM_inTransaction are in itm_user.h */

/*! Begin an outer transaction.
 * This function can return more than once (cf setjump). The result always tells the compiled
 * code what needs to be executed next.
 * \param __td The transaction pointer.
 * \param __mo  The memento pointer.
 * \param __properties A bit mask composed of values from _ITM_codeProperties or-ed together.
 * \param __src The source location.
 * \return A bit mask composed of values from _ITM_actions or-ed together.
 */
_ITM_EXPORT extern uint32_t _ITM_CALL_CONVENTION _ITM_beginOuterTransaction (_ITM_transaction * __td, 
                                                                             _ITM_memento *__mo, 
                                                                             uint32_t __properties,
                                                                             const _ITM_srcLocation *__src);
/*! Begin an inner transaction. Arguments and result as for beginOuterTransaction. */
_ITM_EXPORT extern uint32_t _ITM_CALL_CONVENTION _ITM_beginInnerTransaction (_ITM_transaction * __td, 
                                                                             _ITM_memento *__mo, 
                                                                             uint32_t __properties,
                                                                             const _ITM_srcLocation *__src);
/*! Begin a transaction which may or may not be nested. Arguments and result as for beginOuterTransaction. */
_ITM_EXPORT extern uint32_t _ITM_CALL_CONVENTION _ITM_beginTransaction      (_ITM_transaction * __td, 
                                                                             _ITM_memento *__mo, 
                                                                             uint32_t __properties,
                                                                             const _ITM_srcLocation *__src);

/*! Commit an outer transaction.
 * If this fuction returns the transaction is committed. On a commit
 * failure the commit will not return, but will instead longjump and return from the associated
 * begin*Transaction call.
 * \param __td The transaction pointer.
 * \param __src The source location of the transaction.
 * 
 */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_commitOuterTransaction(_ITM_transaction * __td, const _ITM_srcLocation *__src);
/*! Commit an inner transaction. Arguments and result as for commitOuterTransaction. */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_commitInnerTransaction(_ITM_transaction *, const _ITM_srcLocation *);
/*! Commit a transaction which may or may not be nested. Arguments and result as for commitOuterTransaction. */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_commitTransaction     (_ITM_transaction *, const _ITM_srcLocation *);
/*! Commit all transactions up to and including the one whose memento is passed in. */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_commitUpToTransaction(_ITM_transaction *, 
                                                                        _ITM_memento *, 
                                                                        const _ITM_srcLocation *);

/*! Abort an outer transaction.
 * This function will never return, instead it will longjump and return from the associated
 * begin*Transaction call.
 * \param __td The transaction pointer.
 * \param __reason The reason for the abort.
 * \param __src The source location of the __tm_abort (if abort is called by the compiler).
 */
_ITM_NORETURN (_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_abortOuterTransaction(_ITM_transaction * __td,
                                                                           _ITM_abortReason __reason, 
                                                                           const _ITM_srcLocation * __src));
/*! Abort an inner transaction. Arguments and semantics as for abortOuterTransaction. */
/* Will return if called with uplevelAbort, otherwise it longjumps */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_abortInnerTransaction(_ITM_transaction *, 
                                                                        _ITM_abortReason, 
                                                                        const _ITM_srcLocation *);
/*! Abort a transaction which may or may not be nested. Arguments and semantics as for abortOuterTransaction. */
/* Will return if called with uplevelAbort, otherwise it longjumps */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_abortTransaction(_ITM_transaction *, 
                                                                   _ITM_abortReason, 
                                                                   const _ITM_srcLocation *);
/*! Abort transactions up to and including the given level. 
 * Does not return, but the compiler can't see that (since it calls abortTransaction),
 * and objects if we tag it as NORETURN.
 */
void _ITM_CALL_CONVENTION  _ITM_abortUpToTransaction(_ITM_transaction *, 
                                                     _ITM_memento *, 
                                                     _ITM_abortReason,
                                                     const _ITM_srcLocation *);

/*! Enter an irrevocable mode.
 * If this function returns then execution is now in the new mode, however it's possible that
 * to enter the new mode the transaction must be aborted and retried, in which case this function
 * will not return.
 *
 * \param __td The transaction descriptor.
 * \param __mode The new execution mode.
 * \param __src The source location.
 */
TM_WAIVER
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_changeTransactionMode (_ITM_transaction * __td, 
                                 _ITM_transactionState __mode, const _ITM_srcLocation * __loc);

/* Support macros to generate the multiple data transfer functions we need. 
 * We leave them defined because they are useful elsewhere, for instance when building
 * the vtables we use for mode changes.
 */

# ifndef __cplusplus
/*! Invoke a macro on each of the transfer functions, passing it the
 * result type, function name (without the _ITM_ prefix) and arguments (including parentheses).
 * This can be used to generate all of the function prototypes, part of the vtable,
 * statistics enumerations and so on.
 *
 * Anywhere where the names of all the transfer functions are needed is a good candidate
 * to use this macro.
 */

#  define _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(GENERATE, ACTION, ...)  \
GENERATE (ACTION, uint8_t,U1, __VA_ARGS__)                              \
GENERATE (ACTION, uint16_t,U2, __VA_ARGS__)                             \
GENERATE (ACTION, uint32_t,U4, __VA_ARGS__)                             \
GENERATE (ACTION, uint64_t,U8, __VA_ARGS__)                             \
GENERATE (ACTION, float,F, __VA_ARGS__)                                 \
GENERATE (ACTION, double,D, __VA_ARGS__)                                \
GENERATE (ACTION, long double,E, __VA_ARGS__)                           \
GENERATE (ACTION, __m64,M64, __VA_ARGS__)                               \
GENERATE (ACTION, __m128,M128, __VA_ARGS__)                             \
GENERATE (ACTION, float _Complex, CF, __VA_ARGS__)                      \
GENERATE (ACTION, double _Complex, CD, __VA_ARGS__)                     \
GENERATE (ACTION, long double _Complex, CE, __VA_ARGS__) 
# else
#  define _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(GENERATE, ACTION, ...)  \
GENERATE (ACTION, uint8_t,U1, __VA_ARGS__)                              \
GENERATE (ACTION, uint16_t,U2, __VA_ARGS__)                             \
GENERATE (ACTION, uint32_t,U4, __VA_ARGS__)                             \
GENERATE (ACTION, uint64_t,U8, __VA_ARGS__)                             \
GENERATE (ACTION, float,F, __VA_ARGS__)                                 \
GENERATE (ACTION, double,D, __VA_ARGS__)                                \
GENERATE (ACTION, long double,E, __VA_ARGS__)                           \
GENERATE (ACTION, __m64,M64, __VA_ARGS__)                               \
GENERATE (ACTION, __m128,M128, __VA_ARGS__)
# endif

# define _ITM_FOREACH_MEMCPY(ACTION, ...)                                                            \
ACTION (void, memcpyRnWt, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)           \
ACTION (void, memcpyRnWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRnWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtWn,   (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtWt,   (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtaRWn, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtaRWt, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtaRWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)       \
ACTION (void, memcpyRtaRWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)       \
ACTION (void, memcpyRtaWWn, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtaWWt, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, memcpyRtaWWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)       \
ACTION (void, memcpyRtaWWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)

# define _ITM_FOREACH_MEMSET(ACTION, ...)                                               \
ACTION (void, memsetW, (_ITM_transaction *, void *, int, size_t), __VA_ARGS__)          \
ACTION (void, memsetWaR, (_ITM_transaction *, void *, int, size_t), __VA_ARGS__)        \
ACTION (void, memsetWaW, (_ITM_transaction *, void *, int, size_t), __VA_ARGS__)

#define _ITM_FOREACH_SIMPLE_TRANSFER(ACTION,ARG)           \
    _ITM_FOREACH_SIMPLE_READ_TRANSFER(ACTION,ARG)          \
    _ITM_FOREACH_SIMPLE_WRITE_TRANSFER(ACTION,ARG)

# define _ITM_FOREACH_TRANSFER(ACTION, ARG)     \
 _ITM_FOREACH_SIMPLE_TRANSFER(ACTION, ARG)      \
 _ITM_FOREACH_MEMCPY (ACTION,ARG)               \
 _ITM_FOREACH_LOG_TRANSFER(ACTION,ARG)          \
 _ITM_FOREACH_MEMSET (ACTION,ARG)

#  define _ITM_FOREACH_SIMPLE_READ_TRANSFER(ACTION,ARG)                           \
    _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_ITM_GENERATE_READ_FUNCTIONS, ACTION,ARG) 

#  define _ITM_FOREACH_SIMPLE_WRITE_TRANSFER(ACTION,ARG)                           \
    _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_ITM_GENERATE_WRITE_FUNCTIONS, ACTION,ARG) 

#  define _ITM_FOREACH_SIMPLE_LOG_TRANSFER(ACTION,ARG)                           \
    _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_ITM_GENERATE_LOG_FUNCTIONS, ACTION,ARG) 

#  define _ITM_FOREACH_LOG_TRANSFER(ACTION,ARG)                         \
    _ITM_FOREACH_SIMPLE_LOG_TRANSFER(ACTION,ARG)                        \
    ACTION(void, LB, (_ITM_transaction *, const void*, size_t), ARG)

#  define _ITM_GENERATE_READ_FUNCTIONS(ACTION, result_type, encoding, ARG ) \
    ACTION (result_type, R##encoding,   (_ITM_transaction *, const result_type *), ARG) \
    ACTION (result_type, RaR##encoding, (_ITM_transaction *, const result_type *), ARG) \
    ACTION (result_type, RaW##encoding, (_ITM_transaction *, const result_type *), ARG) \
    ACTION (result_type, RfW##encoding, (_ITM_transaction *, const result_type *), ARG)         

#  define _ITM_GENERATE_WRITE_FUNCTIONS(ACTION, result_type, encoding, ARG ) \
    ACTION (void, W##encoding,  (_ITM_transaction *, result_type *, result_type), ARG) \
    ACTION (void, WaR##encoding,(_ITM_transaction *, result_type *, result_type), ARG) \
    ACTION (void, WaW##encoding,(_ITM_transaction *, result_type *, result_type), ARG)

#  define _ITM_GENERATE_LOG_FUNCTIONS(ACTION, result_type, encoding, ARG ) \
    ACTION (void, L##encoding,   (_ITM_transaction *, const result_type *), ARG) 

# define GENERATE_PROTOTYPE(result, function, args, ARG)    \
    _ITM_EXPORT extern result _ITM_CALL_CONVENTION _ITM_##function args;

_ITM_FOREACH_TRANSFER (GENERATE_PROTOTYPE,dummy)
# undef GENERATE_PROTOTYPE

# 	undef TM_WAIVER
# ifdef __cplusplus
} /* extern "C" */
# endif                          /* __cplusplus */

#endif /* defined(_ITM08_H) */
