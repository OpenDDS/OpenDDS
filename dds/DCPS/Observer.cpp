/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "Observer.h"
#include "DataSampleElement.h"
#include "ReceivedDataElementList.h"
#include <dds/DCPS/transport/framework/ReceivedDataSample.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

Observer::Sample::Sample(DDS::InstanceHandle_t i,
                         DDS::InstanceStateKind instance_state,
                         const DDS::Time_t& t,
                         const SequenceNumber& sn,
                         const void* data)
  : instance_(i)
  , instance_state_(instance_state)
  , timestamp_(t)
  , seq_n_(sn)
  , data_(data)
{}

Observer::~Observer() {}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
