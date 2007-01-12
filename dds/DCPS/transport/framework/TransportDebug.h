// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_TRANSPORT_DEBUG_H
#define TAO_DDS_TRANSPORT_DEBUG_H

#include  "dds/DCPS/dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// Build debug level
#ifndef DDS_BLD_DEBUG_LEVEL
#define DDS_BLD_DEBUG_LEVEL 1 // range: 0-5; 0 being lowest
#endif


#define DDS_RUN_DEBUG_LEVEL ::TAO::DCPS::Transport_debug_level

#define TURN_ON_VERBOSE_DEBUG DDS_RUN_DEBUG_LEVEL = DDS_BLD_DEBUG_LEVEL;
#define TURN_OFF_VERBOSE_DEBUG DDS_RUN_DEBUG_LEVEL = 0;
#define VDBG(DBG_ARGS)
#define VDBG_CODE(LOGIC)
#define VDBG_LVL(DBG_ARGS, LEVEL)


#if DDS_BLD_DEBUG_LEVEL > 0

#define VDBG_CORE(DBG_ARGS) \
if (::TAO::DCPS::Transport_debug_level) ACE_DEBUG(DBG_ARGS)

#define VDBG0(DBG_ARGS)
#define VDBG1(DBG_ARGS)
#define VDBG2(DBG_ARGS)
#define VDBG3(DBG_ARGS)
#define VDBG4(DBG_ARGS)
#define VDBG5(DBG_ARGS)

#if DDS_BLD_DEBUG_LEVEL >=1
#undef VDBG1
#define VDBG1(DBG_ARGS) \
VDBG_CORE(DBG_ARGS)
#endif

#if DDS_BLD_DEBUG_LEVEL >=2
#undef VDBG2
#define VDBG2(DBG_ARGS) \
VDBG_CORE(DBG_ARGS)
#endif

#if DDS_BLD_DEBUG_LEVEL >=3
#undef VDBG3
#define VDBG3(DBG_ARGS) \
VDBG_CORE(DBG_ARGS)
#endif

#if DDS_BLD_DEBUG_LEVEL >=4
#undef VDBG4
#define VDBG4(DBG_ARGS) \
VDBG_CORE(DBG_ARGS)
#endif

#if DDS_BLD_DEBUG_LEVEL >=5
#undef VDBG5
#define VDBG5(DBG_ARGS) \
VDBG_CORE(DBG_ARGS)
#endif

/*
  This is the only debug macro you should be using.
  LEVEL = [0-5], 0 being lowest
*/
#undef VDBG_LVL
#define VDBG_LVL(DBG_ARGS, LEVEL) \
VDBG##LEVEL(DBG_ARGS)

// deprecated
#undef VDBG
#define VDBG(DBG_ARGS) \
VDBG_LVL(DBG_ARGS,5)

//deprecated
// Note that this logic is executed even if TheVerboseDebugFlag is off.
// The only way this doesn't expand is if VERBOSE_DEBUG is not defined,
// which causes all of the debug stuff to be "compiled out".
#undef VDBG_CODE
#define VDBG_CODE(LOGIC) LOGIC

#endif // #if DDS_BLD_DEBUG_LEVEL > 0


namespace TAO
{
  namespace DCPS
  {
    /// Transport Logging verbosity level.
    // This needs to be initialized somewhere.
    extern TAO_DdsDcps_Export unsigned int Transport_debug_level;

  } // namespace TAO
} // namespace DCPS

#endif /* TAO_DDS_TRANSPORT_DEBUG_H */
