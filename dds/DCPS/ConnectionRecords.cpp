/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#ifndef DDS_HAS_MINIMUM_BIT

#include "ConnectionRecords.h"

#include "BuiltInTopicUtils.h"
#include "BuiltInTopicDataReaderImpls.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void WriteConnectionRecords::execute()
{
  DDS::DataReader_var d = bit_sub_->lookup_datareader(BUILT_IN_CONNECTION_RECORD_TOPIC);
  if (!d) {
    return;
  }

  ConnectionRecordDataReaderImpl* dr = dynamic_cast<ConnectionRecordDataReaderImpl*>(d.in());
  if (!dr) {
    return;
  }

  for (ConnectionRecords::const_iterator pos = records_.begin(), limit = records_.end(); pos != limit; ++pos) {
    if (pos->first) {
      dr->store_synthetic_data(pos->second, DDS::NEW_VIEW_STATE);
    } else {
      const DDS::InstanceHandle_t ih = dr->lookup_instance(pos->second);
      dr->set_instance_state(ih, DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DDS_HAS_MINIMUM_BIT */
