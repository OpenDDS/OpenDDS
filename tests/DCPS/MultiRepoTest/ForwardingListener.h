// -*- C++ -*-
//
#ifndef FORWARDINGLISTENER_H
#define FORWARDINGLISTENER_H

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/Condition_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class ForwardingListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  ForwardingListenerImpl( OpenDDS::DCPS::Discovery::RepoKey repo);

  virtual ~ForwardingListenerImpl();

  /// Writer to forward data on.
  void dataWriter( ::DDS::DataWriter_ptr writer);

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
  unsigned int samples_;

  /// Writer to forward data on.
  ::DDS::DataWriter_var dataWriter_;

  /// Lock used by the condition below.
  ACE_SYNCH_MUTEX lock_;

  /// Condition used to signal main processing loop when complete.
  ACE_Condition<ACE_SYNCH_MUTEX> condition_;

  bool complete_;

  /// Repository key that we are attached to.
  OpenDDS::DCPS::Discovery::RepoKey repo_;
};

#endif /* FORWARDINGLISTENER_H  */

