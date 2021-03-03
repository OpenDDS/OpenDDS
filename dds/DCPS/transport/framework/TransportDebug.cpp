/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TransportDebug.h"

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export unsigned int Transport_debug_level = 0;

TransportDebug::TransportDebug()
  : log_messages(false)
{}

OpenDDS_Dcps_Export TransportDebug transport_debug;

} // namespace DCPS
} // namespace OpenDDS
