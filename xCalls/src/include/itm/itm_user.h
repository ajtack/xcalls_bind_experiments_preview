/* INTEL CONFIDENTIAL
 * Copyright 2007-2007 Intel Corporation All Rights Reserved.
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
 * distributed, or disclosed in any way without Intel’s prior express
 * written permission.  No license under any patent, copyright, trade
 * secret or other intellectual property right is granted to or
 * conferred upon you by disclosure or delivery of the Materials,
 * either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must
 * be express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or
 * alter this notice or any other notice embedded in Materials by
 * Intel or Intel’s suppliers or licensors in any way.
 */

/* $Revision: 975 $
 * Update Log
 *
 * Aug 02 2007 JHC: Add tm_waiver declarations to all of these functions when being compiled with a 
 *  compiler which understand the idea.
 * May 16 2007 JHC: Created.
 */

/*!
 * \file itm_user.h
 * \brief User interface header file for the SSG STM/HASTM interface.
 *
 * itm_user.h contains the parts of the interface which may be useful
 * to user code, as against the full interface used by the compiler
 * which is defined in itm.h.
 */

#if !defined(_ITM_USER_H)
# define _ITM_USER_H

# ifdef __cplusplus
extern "C"
{
# endif                          /* __cplusplus */

#include <stddef.h>

#ifndef _WINDOWS
  /* supress warnings on declare __attribute__(regparm(2)) function pointers
   */
#pragma warning( disable : 1287 )
#endif

# if (defined(_TM))
    /* We're being compiled in TM mode, so we want to add the TM Waiver declspec to our
     * declarations here.
     */
#  if (_TM >= 20071219)
    /* This is almost certainly the wrong name for this, but it's what's in the compiler so we have
     * to live with it for now.
     */
#   define TM_WAIVER __declspec (tm_pure)
#  else
#   define TM_WAIVER __declspec (tm_waiver)
#  endif
# else
#  define TM_WAIVER 
# endif

/* __fastcall only makes sense on IA32. We use it on all functions for
 * consistency, though it has no effect on functions which take no
 * arguments.
 */
# if (defined (_M_X64))
#  define _ITM_CALL_CONVENTION
# elif (defined (_WIN32))
#  define _ITM_CALL_CONVENTION __fastcall
# else
    /*Assume GCC like behavior on other OSes */
#  define _ITM_CALL_CONVENTION __attribute__((regparm(2)))
# endif

struct _ITM_transactionS;
//! Opaque transaction descriptor.
typedef struct _ITM_transactionS _ITM_transaction;

typedef void (_ITM_CALL_CONVENTION * _ITM_userUndoFunction)(void *);
typedef void (_ITM_CALL_CONVENTION * _ITM_userCommitFunction)(void *);

//! Opaque transaction tag
typedef unsigned long long _ITM_transactionId;

/** Results from inTransaction */
typedef enum 
{
    outsideTransaction = 0,   /**< So "if (inTransaction(td))" works */
    inRetryableTransaction,
    inIrrevocableTransaction
} _ITM_howExecuting;

/*! \return A pointer to the current transaction descriptor.
 */
TM_WAIVER
extern _ITM_transaction * _ITM_CALL_CONVENTION _ITM_getTransaction (void);

/*! Is the code executing inside a transaction?
 * \param __td The transaction descriptor.
 * \return 1 if inside a transaction, 0 if outside a transaction.
 */
TM_WAIVER
extern _ITM_howExecuting _ITM_CALL_CONVENTION _ITM_inTransaction (_ITM_transaction * __td);

/*! Get the thread number which the TM logging and statistics will be using. */
TM_WAIVER 
extern int _ITM_CALL_CONVENTION _ITM_getThreadnum(void) ;

/*! Add an entry to the user commit action log
 * \param __td The transaction descriptor
 * \param __commit The commit function 
 * \param resumingTransactionId The id of the transaction that must be in the undo path before a commit can proceed.
 * \param __arg The user argument to store in the log entry 
 */
TM_WAIVER 
extern void _ITM_CALL_CONVENTION _ITM_addUserCommitAction (_ITM_transaction * __td, 
                                                       _ITM_userCommitFunction __commit,
                                                       _ITM_transactionId resumingTransactionId,
                                                       void * __arg);


/*! Add an entry to the user undo action log
 * \param __td The transaction descriptor
 * \param __undo Pointer to the undo function
 * \param __arg The user argument to store in the log entry 
 */
TM_WAIVER 
extern void _ITM_CALL_CONVENTION _ITM_addUserUndoAction (_ITM_transaction * __td, 
                                                       const _ITM_userUndoFunction __undo, void * __arg);

/*! User registers routines for cleanup on thread exit
 * \param threadFini The routine to call on thread cleanup
 * \param __arg      An argument to pass to the routine
 * A thread can register several functions
 */
TM_WAIVER 
extern void _ITM_CALL_CONVENTION _ITM_registerThreadFinalization(void (_ITM_CALL_CONVENTION *threadFini)(void *), void *__arg);

/** A transaction Id for non-transactional code.  It is guaranteed to be less 
 * than the Id for any transaction 
 */
#define _ITM_noTransactionId (1) // Id for non-transactional code.

/*! A method to retrieve a tag that uniquely identifies a transaction 
    \param __td The transaction descriptor
 */
TM_WAIVER 
extern _ITM_transactionId _ITM_CALL_CONVENTION _ITM_getTransactionId(_ITM_transaction * __td);

/*! A method to remove any references the library may be storing for a block of memory.
    \param __td The transaction descriptor
    \param __start The start address of the block of memory
    \param __size The size in bytes of the block of memory
 */
TM_WAIVER
extern void _ITM_CALL_CONVENTION _ITM_dropReferences (_ITM_transaction * __td, void * __start, size_t __size);

/*! A method to print error from inside the transaction and exit
    \param errString The error description
    \param exitCode  The exit code
 */
TM_WAIVER
extern void _ITM_CALL_CONVENTION _ITM_userError (const char *errString, int exitCode);

# ifndef _ITM08_H			/* itm08.h will undefine it */
# 	undef TM_WAIVER
# endif
# ifdef __cplusplus
} /* extern "C" */
# endif                          /* __cplusplus */
#endif /* defined (_ITM_USER_H) */
