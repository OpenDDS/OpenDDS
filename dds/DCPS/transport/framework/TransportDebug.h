/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TRANSPORT_DEBUG_H
#define OPENDDS_TRANSPORT_DEBUG_H

#include "dds/DCPS/dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// Build debug level
#ifndef DDS_BLD_DEBUG_LEVEL

// Log range: 0 < 5 for log message
// Log level 6 will output extensive trace message.
#define DDS_BLD_DEBUG_LEVEL 5
#endif

// backwards compatibility macros
#define DDS_RUN_DEBUG_LEVEL OpenDDS::DCPS::Transport_debug_level
#define TURN_ON_VERBOSE_DEBUG DDS_RUN_DEBUG_LEVEL = DDS_BLD_DEBUG_LEVEL;
#define TURN_OFF_VERBOSE_DEBUG DDS_RUN_DEBUG_LEVEL = 0;

/*
  This is the only debug macro you should be using.
  LEVEL = [0-5], 0 being lowest
*/
#define VDBG_LVL(DBG_ARGS, LEVEL) \
  if (LEVEL < OpenDDS::DCPS::Transport_debug_level) ACE_DEBUG(DBG_ARGS);
#define VDBG(DBG_ARGS) \
  VDBG_LVL(DBG_ARGS, 5)

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Transport Logging verbosity level.
// This needs to be initialized somewhere.
extern OpenDDS_Dcps_Export unsigned int Transport_debug_level;

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_TRANSPORT_DEBUG_H */
