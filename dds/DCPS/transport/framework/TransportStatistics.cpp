/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportStatistics.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void
InternalTransportStatistics::reload(RcHandle<ConfigStoreImpl> config_store,
                                    const String& config_prefix)
{
  count_messages_ = config_store->get_boolean((config_prefix + "_COUNT_MESSAGES").c_str(), false);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
