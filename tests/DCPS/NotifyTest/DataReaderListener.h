// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl (DistributedConditionSet_rch dcs,
                          int expected);

  virtual ~DataReaderListenerImpl (void);

  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  long num_reads() const {
    return num_reads_;
  }

  long num_received_dispose() const {
    return num_received_dispose_;
  }

  long num_received_unregister() const {
    return num_received_unregister_;
  }

  void stop () {
    shutdown_ = true;
  }


private:
  DistributedConditionSet_rch dcs_;
  const int expected_;

  DDS::DataReader_var reader_;
  long                  num_reads_;
  long                  num_received_dispose_;
  long                  num_received_unregister_;
  bool                  shutdown_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
