// -*- C++ -*-
//

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
{
public:
  DataReaderListenerImpl ();

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

  virtual void on_subscription_disconnected (
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status);

  virtual void on_subscription_reconnected (
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status);

  virtual void on_subscription_lost (
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus & status);

  long num_arrived() const {
    return num_arrived_;
  }

  long num_rejected() const {
    return num_rejected_max_samples_ + num_rejected_max_instances_
           + num_rejected_max_samples_per_instance_;
  }

  long num_rejected_for_max_samples() const {
    return num_rejected_max_samples_;
  }

  long num_rejected_for_max_instances() const {
    return num_rejected_max_instances_;
  }

  long num_rejected_for_max_samples_per_instance() const {
    return num_rejected_max_samples_per_instance_;
  }

//protected:

  virtual ~DataReaderListenerImpl (void);

private:

  DDS::DataReader_var reader_;
  long                  num_arrived_;
  long                  num_rejected_max_samples_;
  long                  num_rejected_max_instances_;
  long                  num_rejected_max_samples_per_instance_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
