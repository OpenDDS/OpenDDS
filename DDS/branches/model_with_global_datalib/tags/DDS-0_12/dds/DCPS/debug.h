// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_DEBUG_H
#define TAO_DDS_DCPS_DEBUG_H

#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO
{
  namespace DCPS
  {
    /// Logging verbosity level.
    /// set by Service_Participant
    /// value guidelines:
    /// 1 - logs that should happen once per process or are warnings
    /// 2 - logs that should happen once per DDS entity
    /// 4 - logs that are related to administrative interfaces
    /// 6 - logs that should happen every Nth sample write/read
    /// 8 - logs that should happen once per sample write/read
    /// 10 - logs that may happen more than once per sample write/read
    extern TAO_DdsDcps_Export unsigned int DCPS_debug_level;

    /// The proper way to set the DCPS_debug_level.
    /// This function allows for possible side-effects of setting the level.
    extern void TAO_DdsDcps_Export set_DCPS_debug_level(unsigned int lvl);

  } // namespace TAO
} // namespace DCPS

#endif /* TAO_DDS_DCPS_DEBUG_H */

