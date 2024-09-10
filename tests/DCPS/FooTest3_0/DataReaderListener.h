/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>

#include <tests/Utils/DistributedConditionSet.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                         long num_writes);

  virtual ~DataReaderListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  long samples_read() const {
    return samples_read_;
  }

  long samples_disposed() const {
    return samples_disposed_;
  }

protected:
  void increment_samples_read()
  {
    ++samples_read_;
    if (samples_read_ == num_writes_) {
      dcs_->post("sub", "done");
    }
  }

  DistributedConditionSet_rch dcs_;
  long                 num_writes_;
  long                 samples_read_;
  long                 samples_disposed_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
