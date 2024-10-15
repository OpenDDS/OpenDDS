/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>
#include <ctime>
#include "ReliabilityTypeSupportImpl.h"
#include <tests/Utils/DistributedConditionSet.h>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(DistributedConditionSet_rch dcs);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader, const DDS::SampleLostStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader, const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status
  );

  void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/,
                                    const DDS::RequestedDeadlineMissedStatus& /*status*/) {}

  void on_liveliness_changed(DDS::DataReader_ptr /*reader*/,
                             const DDS::LivelinessChangedStatus& /*status*/) {}

  void on_subscription_matched(DDS::DataReader_ptr /*reader*/,
                               const DDS::SubscriptionMatchedStatus& /*status*/) {}

  unsigned long long sample_count() { return sample_count_; }
  unsigned long long expected_count() { return expected_count_; }

  void set_sleep_length(unsigned long long sleep_length) { sleep_length_ = sleep_length;};
  void set_num_sleeps(unsigned long long num_sleeps) { num_sleeps_ = num_sleeps;};

  protected:
  virtual void take_samples(
    Reliability::MessageDataReader_var reader_i
  ) = 0;

  void on_sample(Reliability::Message& msg);

  DistributedConditionSet_rch dcs_;
  unsigned long long sample_count_;
  unsigned long long expected_count_;
  long expected_seq_;
  unsigned long long sleep_length_;
  unsigned long long num_sleeps_;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
