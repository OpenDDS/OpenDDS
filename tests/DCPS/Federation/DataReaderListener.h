// -*- C++ -*-
//
#ifndef DATAREADERLISTENER_H
#define DATAREADERLISTENER_H

#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/Condition_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl( int expected);

  virtual ~DataReaderListenerImpl();

  /// Wait for signal from the receiving thread executing on_data_available().
  void waitForCompletion();

  virtual void on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
  );

 virtual void on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  );

  virtual void on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  );

  virtual void on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
  );

  virtual void on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  );

  virtual void on_data_available(
    ::DDS::DataReader_ptr reader
  );

  virtual void on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  );

private:
  /// Number samples received.
  unsigned int samples_;

  /// Number of samples expected.
  unsigned int expected_;

  /// Lock used by the condition below.
  ACE_SYNCH_MUTEX lock_;

  /// Condition used to signal main processing loop when complete.
  ACE_Condition<ACE_SYNCH_MUTEX> condition_;
};

#endif /* DATAREADERLISTENER_H  */

