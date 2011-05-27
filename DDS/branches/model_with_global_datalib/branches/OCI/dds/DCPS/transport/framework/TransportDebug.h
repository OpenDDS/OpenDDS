// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_TRANSPORT_DEBUG_H
#define TAO_DDS_TRANSPORT_DEBUG_H

#include  "dds/DCPS/dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// Do not build any of the verbose debug code in non debug builds.
#if !defined (ACE_NDEBUG)
#define DCPS_TRANS_VERBOSE_DEBUG
#endif

#ifndef DCPS_TRANS_VERBOSE_DEBUG
#define TURN_ON_VERBOSE_DEBUG
#define TURN_OFF_VERBOSE_DEBUG
#define VDBG(DBG_ARGS)
#define VDBG_CODE(LOGIC)

#else

// TBD - Later we might have multiple levels of debug - like DCPS/debug.h
#define TURN_ON_VERBOSE_DEBUG  ::TAO::DCPS::Transport_debug_level = 1;
#define TURN_OFF_VERBOSE_DEBUG ::TAO::DCPS::Transport_debug_level = 0;

#define VDBG(DBG_ARGS) \
if (::TAO::DCPS::Transport_debug_level) ACE_DEBUG(DBG_ARGS)

// Note that this logic is executed even if TheVerboseDebugFlag is off.
// The only way this doesn't expand is if VERBOSE_DEBUG is not defined,
// which causes all of the debug stuff to be "compiled out".
#define VDBG_CODE(LOGIC) LOGIC

#endif


namespace TAO
{
  namespace DCPS
  {
    /// Transport Logging verbosity level.
    /// T
    extern TAO_DdsDcps_Export unsigned int Transport_debug_level;

  } // namespace TAO
} // namespace DCPS

#endif /* TAO_DDS_TRANSPORT_DEBUG_H */

