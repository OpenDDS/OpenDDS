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
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl();

  virtual ~DataReaderListenerImpl(void);

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

  virtual void on_subscription_disconnected(
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus& status);

  virtual void on_subscription_reconnected(
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus& status);

  virtual void on_subscription_lost(
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus& status);

  long num_reads() const {
    return num_reads_;
  }

  void set_builtin_datareader(DDS::DataReader_ptr builtin);

  bool read_bit_instance();

private:

  DDS::DataReader_var reader_;
  long num_reads_;
  ::DDS::InstanceHandle_t publication_handle_;
  ::DDS::InstanceHandle_t post_restart_publication_handle_;
  DDS::DataReader_var builtin_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
