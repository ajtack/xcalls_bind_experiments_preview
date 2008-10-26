/* INTEL CONFIDENTIAL
 * Copyright 2007-2007 Intel Corporation All Rights Reserved. 
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or disclosed
 * in any way without Intel’s prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

/* This is a minimal stdint.h so that we can compile on Windows, which doesn't provide
 * <stdint.h> :-(
 * It is incomplete and only provides the types we need to compile our library.
 */

/* Update Log
 *
 * Mar 15 2007 JHC: Created.
 */

#ifndef _ITM_STDINT_INCLUDED
# define _ITM_STDINT_INCLUDED

# if (defined (_WIN32))
typedef unsigned char uint8_t;
typedef signed   char int8_t;

typedef unsigned short uint16_t;
typedef signed   short int16_t;

typedef unsigned int uint32_t;
typedef signed   int int32_t;

typedef unsigned long long uint64_t;
typedef signed   long long int64_t;

#  if (!defined (_M_X64))
typedef uint32_t uintptr_t;
typedef int32_t  intptr_t;
#  endif

# else
/* Aha, a real system which should have its own stdint.h. Use it ! */
#  include <stdint.h>
# endif
#endif /* Idempotence */
