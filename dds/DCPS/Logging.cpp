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
                                      const TimeDuration& duration,
                                      size_t messages_sent,
                                      size_t messages_received)
{
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO local: %C remote: %C time: %d.%06u msg_sent: %B msg_recv: %B activity: %C\n",
             DCPS::LogGuid(local).c_str(), DCPS::LogGuid(remote).c_str(),
             duration.value().sec(), duration.value().usec(),
             messages_sent, messages_received, activity));

}

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
