/* INTEL CONFIDENTIAL
 * Copyright 2006-2007 Intel Corporation All Rights Reserved.
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

/*
 * Header file for the SSG STM/HASTM interface.
 * $Revision: 1152 $
 *
 * Update Log
 *
 * June 24 2007 SB: Remove ABI 0.6 code
 * May 17 2007 JHC: Created as a simple version switch for the different ABIs.
 *    The previous version becomes itm06.h
 */

/*!
 * \file itm.h
 * \brief Header file for the SSG STM/HASTM interface.
 *
 * itm.h just checks the version number (or defaults it if it is unset) and then #includes
 * the appropriate header file for the requested ABI version.
 */

#if !defined(_ITM_H)                            /* Idempotence */
# define _ITM_H

/* Choose the default version of the ABI. */
# if (!defined (_ITM_VERSION_NO))
#  define _ITM_VERSION_NO 80
# endif

# if (_ITM_VERSION_NO <= 89)
#  include "itm08.h"
# else
#  error Unknown ITM version number _ITM_VERSION_NO
# endif

#endif /* defined(_ITM_H) */
