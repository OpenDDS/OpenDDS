/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ConnectionRecords.h"

#if OPENDDS_CONFIG_BUILT_IN_TOPICS

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void WriteConnectionRecords::execute()
{
  RcHandle<BitSubscriber> bit_sub = bit_sub_.lock();
  if (!bit_sub) {
    return;
  }

  for (ConnectionRecords::const_iterator pos = records_.begin(), limit = records_.end(); pos != limit; ++pos) {
    if (pos->first) {
      bit_sub->add_connection_record(pos->second, DDS::NEW_VIEW_STATE);
    } else {
      bit_sub->remove_connection_record(pos->second);
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
