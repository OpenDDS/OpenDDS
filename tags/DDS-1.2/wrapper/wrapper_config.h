// -*- C++ -*-

//=============================================================================
/**
 *  @file    wrapper_config.h
 *
 *  $Id$
 *
 * @author   Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _DDS_WRAPPER_CONFIG_H_
#define _DDS_WRAPPER_CONFIG_H_

#if defined (OPEN_SPLICE_CONFIG)

# if defined (OPEN_DDS_CONFIG)
# error Only one implementation config flag should be uncommented
# endif

#include "ccpp.h"

#elif !defined (OPEN_DDS_CONFIG)
#define OPEN_DDS_CONFIG
#endif

#if defined (OPEN_DDS_CONFIG)

# if defined (OPEN_SPLICE_CONFIG)
# error Only one implementation config flag should be uncommented
# endif

#endif /* if defined */

#endif /* _DDS_WRAPPER_CONFIG_H_ */
