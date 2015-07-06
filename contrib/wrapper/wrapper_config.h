// -*- C++ -*-

//=============================================================================
/**
 *  @file    wrapper_config.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_CONFIG_H_
#define DDS_WRAPPER_CONFIG_H_

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

#include <dds/DCPS/Marked_Default_Qos.h>

#endif /* if defined */

#endif /* DDS_WRAPPER_CONFIG_H_ */
