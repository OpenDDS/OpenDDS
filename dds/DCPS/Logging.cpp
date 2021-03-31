/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h"

#include "Logging.h"

#include "GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void OpenDDS_Dcps_Export log_progress(const char* activity,
                                      const GUID_t& local,
                                      const GUID_t& remote,
                                      const MonotonicTime_t& start_time,
                                      const GUID_t& reference)
{
  ACE_DEBUG((LM_INFO, "(%P|%t) {transport_debug.log_progress} local: %C remote: %C reference: %C time(ms): %Lu activity: %C\n",
             DCPS::LogGuid(local).c_str(), DCPS::LogGuid(remote).c_str(), DCPS::LogGuid(reference).c_str(),
             duration_to_time_value(MonotonicTimePoint::now().to_monotonic_time() - start_time).msec(),
             activity));
}

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
