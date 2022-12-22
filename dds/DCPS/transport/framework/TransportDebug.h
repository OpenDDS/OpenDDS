/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTDEBUG_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTDEBUG_H

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

class OpenDDS_Dcps_Export TransportDebug {
public:
  TransportDebug();

  /// Log all RTPS messages sent or recieved.
  bool log_messages;
  /// Log progress for RTPS entity discovery and association.
  bool log_progress;
  /// Log received RTPS messages that were dropped.
  bool log_dropped_messages;
  /**
   * Log non-final RTPS messages send or received. Useful to gauge lost
   * messages and resends.
   */
  bool log_nonfinal_messages;
  /**
   * Log message fragment reassembly process for transports where that applies.
   * Also logged when the transport debug level is set to the most verbose.
   */
  bool log_fragment_storage;
  /// Log number of associations and pending associations of RTPS entities.
  bool log_remote_counts;
};
extern OpenDDS_Dcps_Export TransportDebug transport_debug;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_TRANSPORT_DEBUG_H */
