/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NOP_DATA_READER_LISTENER_H
#define OPENDDS_DCPS_NOP_DATA_READER_LISTENER_H

#include "LocalObject.h"

#include <dds/Versioned_Namespace.h>

#include <dds/DdsDcpsSubscriptionC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class NopDataReaderListener : public virtual LocalObject<DDS::DataReaderListener> {
public:
  void on_requested_deadline_missed(
    DDS::DataReader_ptr /*reader*/,
    const DDS::RequestedDeadlineMissedStatus& /*status*/)
  {
  }

  void on_requested_incompatible_qos(
    DDS::DataReader_ptr /*reader*/,
    const DDS::RequestedIncompatibleQosStatus& /*status*/)
  {
  }

  void on_sample_rejected(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SampleRejectedStatus& /*status*/)
  {
  }

  void on_liveliness_changed(
    DDS::DataReader_ptr /*reader*/,
    const DDS::LivelinessChangedStatus& /*status*/)
  {
  }

  void on_data_available(DDS::DataReader_ptr /*reader*/)
  {
  }

  void on_subscription_matched(
    DDS::DataReader_ptr /*reader*/,
    const DDS::SubscriptionMatchedStatus& /*status*/)
  {
  }

  void on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/)
  {
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_NOP_DATA_READER_LISTENER_H */
