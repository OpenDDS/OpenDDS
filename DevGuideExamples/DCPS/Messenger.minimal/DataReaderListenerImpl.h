/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionS.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl();

  void on_data_available(DDS::DataReader_ptr reader);

  void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/,
                                    const DDS::RequestedDeadlineMissedStatus& /*status*/) {}

  void on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/,
                                     const DDS::RequestedIncompatibleQosStatus& /*status*/) {}

  void on_sample_rejected(DDS::DataReader_ptr /*reader*/,
                          const DDS::SampleRejectedStatus& /*status*/) {}

  void on_liveliness_changed(DDS::DataReader_ptr /*reader*/,
                             const DDS::LivelinessChangedStatus& /*status*/) {}

  void on_subscription_matched(DDS::DataReader_ptr /*reader*/,
                               const DDS::SubscriptionMatchedStatus& /*status*/) {}

  void on_sample_lost(DDS::DataReader_ptr /*reader*/,
                      const DDS::SampleLostStatus& /*status*/) {}

  int sample_count;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
